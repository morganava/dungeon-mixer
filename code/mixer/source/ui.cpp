#include "ui.hpp"
#include "utilities.hpp"

namespace tabletopify
{
    static ui* g_ui = nullptr;

    ui* get_ui()
    {
        MX_ASSERT(g_ui != nullptr);
        return g_ui;
    }

    void init_ui()
    {
        MX_ASSERT(g_ui == nullptr);
        g_ui = new ui();
        g_ui->initialize();
    }
    void free_ui()
    {
        g_ui->finish();
        delete g_ui;
        g_ui = nullptr;
    }

    void ui::initialize()
    {
    }

    void ui::finish()
    {

    }

    // immmediate mode gui
    namespace imgui
    {
        template<typename TYPE>
        class singleton
        {
        public:
            static constexpr TYPE* addr()
            {
                return &data.value;
            }

            static constexpr TYPE& get()
            {
                return data.value;
            }

            static void assign(TYPE&& that)
            {
                new (addr()) TYPE(std::move(that));
            }

            static constexpr void free()
            {
                data.value.~TYPE();
            }

        private:
            union singleton_data
            {
                TYPE value;
                uint8_t raw[sizeof(TYPE)] = {};
            };
            static singleton_data data;
        };
        template<typename TYPE>
        typename singleton<TYPE>::singleton_data singleton<TYPE>::singleton::data = {};

        template<typename RETVAL, typename... ARGS>
        class callback
        {
        public:
            ~callback()
            {
                delete_func();
            }

            constexpr callback& operator=(RETVAL(*function)(ARGS...))
            {
                exec_func = function;
                delete_func = [](){};
                return *this;
            }

            template<typename FUNCTOR>
            callback& operator=(FUNCTOR&& functor)
            {
                singleton<FUNCTOR>::assign(std::move(functor));
                set_callbacks<FUNCTOR>();
                return *this;
            }

            constexpr RETVAL operator()(ARGS... args)
            {
                return exec_func(std::forward<ARGS>(args)...);
            }
        private:

            template<typename FUNCTOR>
            constexpr void set_callbacks()
            {
                delete_func = []() -> void
                {
                    singleton<FUNCTOR>::free();
                };

                exec_func = [](ARGS... args) -> RETVAL
                {
                    return singleton<FUNCTOR>::get()(args...);
                };
            }

            void(*delete_func)() = nullptr;
            RETVAL(*exec_func)(ARGS...) = nullptr;
        };

        template<typename Return, typename... Args>
        auto make_callback_impl(Return(*)(Args...))
        {
            return callback<Return,Args...>();
        }

        template<typename Func>
        auto make_callback()
        {
            Func* func = nullptr;
            return make_callback_impl(func);
        }

        struct vec2
        {
            float x = 0.0f;
            float y = 0.0f;

            constexpr vec2() = default;
            constexpr vec2(const vec2&) = default;
            constexpr vec2(vec2&&) = default;
            
            constexpr vec2(float x, float y)
            : x(x)
            , y(y)
            { }
            
            constexpr vec2& operator=(const vec2&) = default;

            constexpr vec2 operator+(const vec2& that) const
            {
                return vec2(this->x + that.x, this->y + that.y);
            }

            constexpr vec2 operator-(const vec2& that) const
            {
                return vec2(this->x - that.x, this->y - that.y);
            }

            constexpr vec2 operator*(float f) const
            {
                return vec2(this->x * f, this->y * f);
            }
        };

        struct rect
        {
            vec2 position;
            vec2 size;
        };

        template<typename T, size_t LEN>
        struct fixed_stack
        {
            constexpr void push(T val)
            {
                _buffer[_idx++] = val;
            }

            constexpr T peek() const
            {
                assert(_idx > 0);
                return _buffer[_idx - 1];
            }

            constexpr void pop()
            {
                assert(_idx > 0);
                --_idx;
            }

            constexpr size_t size() const
            {
                return _idx;
            }

        private:
            size_t _idx = 0;
            T _buffer[LEN] = {};
        };

        struct bounds_callback
        {
            constexpr rect eval(const rect& desired) const
            {
                return callback(element, desired);
            }

            rect (*callback)(void*, const rect&) = nullptr;
            void* element = nullptr;
        };

        class context
        {
        public:
            context(NVGcontext* nvgContext)
            : _nvgContext(nvgContext)
            { }

            ~context()
            {
                if (_bounds_stack.size() != 0) 
                {
                       MX_LOG_INFO("bounds.size(): {}", _bounds_stack.size());
                }
                MX_ASSERT(_bounds_stack.size() == 0);
            }

            void push_bounds(const bounds_callback& bounds)
            {
                // MX_DEBUG_TRACE("push_bounds");
                _bounds_stack.push(bounds);
            }

            bounds_callback peek_bounds() const
            {
                // MX_DEBUG_TRACE("peek_bounds");
                auto retval = _bounds_stack.peek();
                return retval;
            }

            void pop_bounds()
            {
                // MX_DEBUG_TRACE("pop_bounds");
                _bounds_stack.pop();
            }

            NVGcontext* nvgcontext()
            {
                return _nvgContext;
            }

        private:
            fixed_stack<bounds_callback, 16> _bounds_stack;
            NVGcontext* _nvgContext = nullptr;
        };

        /// layout elements

        // callbacks
        template<typename T>
        void construct(T& self, auto&& func)
        {
            MX_LOG_INFO("Construct : {}", typeid(T()).name());
            func(self);
            pre_render(self);
            rect desired = desired_bounds(self);
            rect bounds = self._ctx.peek_bounds().eval(desired);
            self._bounds = bounds;
            MX_LOG_INFO("render bounds: {},{} {},{}", bounds.position.x, bounds.position.y, bounds.size.x, bounds.size.y);
            render(self, self._ctx.nvgcontext(), bounds);
            post_render(self);
        }

        template<typename T>
        void destroy(T& elem) {}
        template<typename T>
        void pre_render(T& elem) {}
        template<typename T>
        rect desired_bounds(const T& elem) {return {};};
        template<typename T>
        void render(const T& elem, NVGcontext* nvgctx, const rect& bounds) {}
        template<typename T>
        void post_render(T& elem)
        {
            elem._ctx.push_bounds(
            {
                [](void*, const rect&) -> rect
                {
                    MX_ASSERT(!"should not be called");
                    return {};
                },
                nullptr
            });
        }

        // base element

        template<typename T>
        struct element
        {
            element() = delete;
            element(const element&) = delete;
            element(element&&) = delete;
            element(context& ctx)
            : _ctx(ctx)
            { }

            ~element()
            {
                _ctx.pop_bounds();

                destroy(static_cast<T&>(*this));
            }

            context& _ctx;
            rect _bounds;
        };

        struct elem_group : public element<elem_group>
        {
            elem_group(context& ctx, auto&& func)
            : element(ctx)
            { 
                construct(*this, std::forward<decltype(func)>(func));
            }
            size_t _vertical_offset = 0;
        };

        template<>
        void post_render<elem_group>(elem_group& self)
        {
            self._ctx.push_bounds(
            {
                [](void* ptr, const rect& desired) -> rect
                {
                    elem_group* eg = reinterpret_cast<elem_group*>(ptr);
                    rect retval = 
                    {
                        {eg->_bounds.position.x, eg->_bounds.position.y + eg->_vertical_offset},
                        {eg->_bounds.size.x, desired.size.y}
                    };
                    eg->_vertical_offset += desired.size.y;
                    return retval;
                },
                &self
            });
        }

        struct elem_static_row : public element<elem_static_row>
        {
            elem_static_row(context& ctx, auto&& func)
            : element(ctx)
            { 
                construct(*this, std::forward<decltype(func)>(func));
            }

            std::vector<float> _widths;
            float _height = 0.0f;
            float _horizontal_offset = 0.0f;

            friend void post_render<elem_static_row>(elem_static_row&);
        };

        template<>
        rect desired_bounds<elem_static_row>(const elem_static_row& self)
        {
            rect retval = {};
            retval.size.y  = self._height;
            return retval;
        }

        template<>
        void post_render<elem_static_row>(elem_static_row& self)
        {
            self._ctx.push_bounds(
            {
                [](void* ptr, const rect&) -> rect
                {
                    elem_static_row* esr = reinterpret_cast<elem_static_row*>(ptr);
                    float w = esr->_widths.front();
                    esr->_widths.erase(esr->_widths.begin());

                    rect retval = 
                    {
                        {esr->_bounds.position.x + esr->_horizontal_offset, esr->_bounds.position.y},
                        {w, esr->_bounds.size.y}
                    };

                    esr->_horizontal_offset += w;

                    return retval;
                },
                &self
            });                
        }

        /// draw elements

        struct elem_debug : public element<elem_debug>
        {
            elem_debug(context& ctx, auto&& func)
            : element(ctx)
            { 
                construct(*this, std::forward<decltype(func)>(func));
            }
        };

        template<>
        void render<elem_debug>(const elem_debug& elem, NVGcontext* ctx, const rect& bounds)
        {
            nvgBeginPath(ctx);
            nvgRect(ctx, bounds.position.x, bounds.position.y, bounds.size.x, bounds.size.y);
            nvgFillColor(ctx, nvgRGB(0, 0, 0));
            nvgFill(ctx);

            nvgBeginPath(ctx);
            nvgRect(ctx, bounds.position.x + 1.0f, bounds.position.y + 1.0f, bounds.size.x - 2.0f, bounds.size.y - 2.0f);
            nvgFillColor(ctx, nvgRGB(0, 0xff, 0xff));
            nvgFill(ctx);     
        }


    } // imgui namespace

    #define mxCircle(...)\
    if(imgui::circle_element __circle ## __LINE__([=](auto& self) {\
        style_t style = g_ui->peek_style();\
        __VA_ARGS__\
        self.render(ctx, style);\
        return style;\
    }); true)

    void ui::update(NVGcontext* ctx, float dt, int32_t width, int32_t height)
    {
        MX_LOG_INFO("--Begin Update--");

        imgui::context imgui_ctx(ctx);
        imgui::rect bounds = {{0, 0}, {(float)width, (float)height}};
        imgui_ctx.push_bounds(
        {
            [](void* bounds, const imgui::rect&) -> imgui::rect 
            {
                return *reinterpret_cast<imgui::rect*>(bounds);
            },
            &bounds
        });

        if(imgui::elem_group root_group(imgui_ctx, [&](auto& self)
        {
        }); true)
        {
            if(imgui::elem_static_row dyn_row(imgui_ctx, [&](auto& self)
            {
                auto& width = self._widths;
                auto& height = self._height;
                width = {20.0f, 50.0f};
                height = 100;
            }); true)
            {
                imgui::elem_debug(imgui_ctx, [](auto& self){});
                imgui::elem_debug(imgui_ctx, [](auto& self){});
            }
#if 1
            if(imgui::elem_static_row dyn_row(imgui_ctx, [&](auto& self)
            {
                auto& width = self._widths;
                auto& height = self._height;
                width = {25.0f, 20.0f, 50.0f, 100.0f};
                height = 200;
            }); true)
            {
                imgui::elem_debug(imgui_ctx, [](auto& self){});
                imgui::elem_debug(imgui_ctx, [](auto& self){});
                imgui::elem_debug(imgui_ctx, [](auto& self){});
                imgui::elem_debug(imgui_ctx, [](auto& self){});
            }
#endif
        }

        imgui_ctx.pop_bounds();

        MX_LOG_INFO("--End Update--");
    }
}
