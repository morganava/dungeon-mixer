#pragma once

namespace tabletopify
{
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

            rect (*callback)(void*, const rect&) = [](void*, const rect&) -> rect {MX_ASSERT(!"Should Not Be Called"); return {};};
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

            constexpr void push_bounds(const bounds_callback& bounds)
            {
                // MX_DEBUG_TRACE("push_bounds");
                _bounds_stack.push(bounds);
            }

            constexpr bounds_callback peek_bounds() const
            {
                // MX_DEBUG_TRACE("peek_bounds");
                auto retval = _bounds_stack.peek();
                return retval;
            }

            constexpr void pop_bounds()
            {
                // MX_DEBUG_TRACE("pop_bounds");
                _bounds_stack.pop();
            }

            constexpr NVGcontext* nvgcontext()
            {
                return _nvgContext;
            }

        private:
            fixed_stack<bounds_callback, 16> _bounds_stack;
            NVGcontext* _nvgContext = nullptr;
        };

        // base element

        template<typename T>
        struct element
        {
            element() = delete;
            element(const element&) = delete;
            element(element&&) = delete;
            constexpr element(context& ctx)
            : _ctx(ctx)
            { }

            ~element()
            {
                _ctx.pop_bounds();
            }

            void update_render_bounds()
            {
                _bounds = _ctx.peek_bounds().eval(_bounds);
            }

            void render()
            {
                static_cast<T*>(this)->render(_ctx.nvgcontext(), _bounds);
            }

            context& _ctx;
            rect _bounds;
        };

        struct elem_group : public element<elem_group>
        {
            using element::element;
            using element::render;

            void pre_render()
            {

            }

            void render(NVGcontext* ctx, const rect& bounds)
            {

            }

            void post_render()
            {
                _ctx.push_bounds(
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
                    this
                });
            }

            size_t _vertical_offset = 0;
        };

        struct elem_static_row : public element<elem_static_row>
        {
            using element::element;
            using element::render;

            void pre_render()
            {

            }

            void render(NVGcontext* ctx, const rect& bounds)
            {
                // MX_LOG_INFO("RENDERING STATIC_ROW : position {},{} size {},{}", bounds.position.x, bounds.position.y, bounds.size.x, bounds.size.y);
            }

            void post_render()
            {
                _ctx.push_bounds(
                {
                    [](void* ptr, const rect&) -> rect
                    {
                        elem_static_row* esr = static_cast<elem_static_row*>(ptr);
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
                    this
                });
            }

            std::vector<float> _widths;
            float _height = 0.0f;
            float _horizontal_offset = 0.0f;            
        };

        struct elem_debug : public element<elem_debug>
        {
            using element::element;
            using element::render;

            void pre_render()
            {

            }

            void render(NVGcontext* ctx, const rect& bounds)
            {
                // MX_LOG_INFO("RENDERING DEBUG : position {},{} size {},{}", bounds.position.x, bounds.position.y, bounds.size.x, bounds.size.y);

                nvgBeginPath(ctx);
                nvgRect(ctx, bounds.position.x, bounds.position.y, bounds.size.x, bounds.size.y);
                nvgFillColor(ctx, nvgRGB(0, 0, 0));
                nvgFill(ctx);

                nvgBeginPath(ctx);
                nvgRect(ctx, bounds.position.x + 1.0f, bounds.position.y + 1.0f, bounds.size.x - 2.0f, bounds.size.y - 2.0f);
                nvgFillColor(ctx, nvgRGB(0, 0xff, 0xff));
                nvgFill(ctx);   
            }

            void post_render()
            {
                _ctx.push_bounds({});
            }
        };
    } // imgui namespace


#define MX_ELEMENT_CODEGEN(X, ...)\
    if (imgui::X group##__LINE__(imgui_ctx); \
        [&](auto& self) -> bool {\
            __VA_ARGS__\
            self.update_render_bounds(); self.pre_render(); self.render(); self.post_render();\
            return true;\
        }(group##__LINE__))

#define mxGroup(...) MX_ELEMENT_CODEGEN(elem_group, __VA_ARGS__)
#define mxStaticRow(...) MX_ELEMENT_CODEGEN(elem_static_row, __VA_ARGS__)
#define mxDebug(...) MX_ELEMENT_CODEGEN(elem_debug, __VA_ARGS__)

}