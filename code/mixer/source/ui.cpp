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
        // low cost calbacks for use with immediate mode ui
        template<typename Return, typename... Args>
        class callback
        {
            typedef Return(*function_t)(void*, Args...) ;        
            typedef void(*destroy_t)(void*);    
        public:
            callback() = default;
            callback(const callback&) = delete;
            callback(callback&& that)
            : _function(that._function)
            , _destroy(that._destroy)
            , _data(that._data)
            {
                that._function = nullptr;
                that._destroy = nullptr;
                that._data = nullptr;
            }

            ~callback()
            {
                if(_destroy)
                {
                    _destroy(_data);
                }
            }

            callback& operator=(callback&& that)
            {
                _function = that._function;
                _destroy = that._destroy;
                _data = that._data;

                that._function = nullptr;
                that._destroy = nullptr;
                that._data = nullptr;
            }

            template<typename T>
            callback& operator=(T&& lam)
            {
                if (_destroy != nullptr)
                {
                    _destroy(_data);
                }

                const function_t function = [](void* plam, Args... args) -> Return 
                {
                    return (*reinterpret_cast<T*>(plam))(args...);
                };
                const destroy_t destroy = [](void* plam)
                {
                    reinterpret_cast<T*>(plam)->~T();
                };

                static uint8_t slam[sizeof(T)];
                new (slam) T(std::forward<T>(lam));
                void* data = slam;

                this->_function = function;
                this->_destroy = destroy;
                this->_data = data;

                return *this;
            }

            Return operator()(Args... args) const
            {
                if(_function) {
                    _function(_data, args...);
                }
            }

            operator bool() const
            {
                return _function != nullptr;
            }
        private:
            function_t _function = nullptr;
            destroy_t _destroy = nullptr;
            void* _data =  nullptr; 
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
            
            vec2& operator=(const vec2&) = default;

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

#if 0
        struct circle_element
        {
            circle_element() = delete;
            circle_element(auto&& func)
            {
                style_t s = func(*this);
                g_ui->push_style(s);
            }

            ~circle_element()
            {
                g_ui->pop_style();
            }

            void render(NVGcontext* ctx, const style_t& style)
            {
                nvgBeginPath(ctx);
                nvgCircle(ctx, style.x, style.y, std::min(style.width, style.height));
                nvgFillColor(ctx, style.fill_color);
                nvgFill(ctx);
            }
        };
#endif    

        class context
        {
        public:
            context(NVGcontext* nvgContext)
            : _nvgContext(nvgContext)
            { }

            ~context()
            {
                // MX_LOG_INFO("bounds.size(): {}", _bounds_stack.size());   
                MX_ASSERT(_bounds_stack.size() == 0);
            }

            void push_bounds(const rect& bounds)
            {
                // MX_DEBUG_TRACE("push_bounds");
                _bounds_stack.push_back(bounds);
            }

            rect peek_bounds() const
            {
                // MX_DEBUG_TRACE("peek_bounds");
                rect retval = _bounds_stack.back();
                return retval;
            }

            void pop_bounds()
            {
                // MX_DEBUG_TRACE("pop_bounds");
                _bounds_stack.pop_back();
            }

            NVGcontext* nvgcontext()
            {
                return _nvgContext;
            }

        private:
            std::vector<rect> _bounds_stack;
            NVGcontext* _nvgContext = nullptr;
        };

        /// layout elements

        // callbacks
        template<typename T>
        void pre_render(const T& elem) {};
        template<typename T>
        void render(const T& elem, NVGcontext* nvgctx, const rect& bounds) {}
        template<typename T>
        void post_render(const T& elem) {};

        // base element

        template<typename T>
        struct element
        {
            element() = delete;
            ~element()
            {
                _ctx.pop_bounds();
            }

            element(context& ctx, auto&& func)
            : _ctx(ctx)
            {
                auto& self = static_cast<T&>(*this);
                func(self);
                render(self, _ctx.nvgcontext(), _ctx.peek_bounds());
            }

        protected:
            context& _ctx;
        };

        struct elem_group
        {
            elem_group() = delete;
            ~elem_group()
            {
                _ctx.pop_bounds();
            }
            elem_group(context& ctx, auto&& func)
            : _ctx(ctx)
            {
                func(*this);
                render(_ctx.nvgcontext(), _ctx.peek_bounds());
            }

            void render(NVGcontext* ctx, const rect& bounds)
            {

            }
        private:
            context& _ctx;
        };

        struct elem_static_row
        {
            elem_static_row() = delete;
            ~elem_static_row()
            {
                _ctx.pop_bounds();

                auto bound = _ctx.peek_bounds();
                bound.position.y += _height;
                bound.size.y -= _height;
                _ctx.pop_bounds();
                _ctx.push_bounds(bound);
            }            
            elem_static_row(context& ctx, auto&& func)
            : _ctx(ctx)
            {
                func(*this);
                auto bound = ctx.peek_bounds();
                render(ctx.nvgcontext(), bound);
                
                rect child_bounds[_widths.size()];
                float x_offset = 0.0f;
                for(size_t i = 0; i < _widths.size(); ++i)
                {
                    float w = _widths[i];
                    child_bounds[i] = {{bound.position.x + x_offset, bound.position.y},{w, _height}};
                    x_offset += w;
                }

                ctx.push_bounds({bound.position, {x_offset, _height}});

                for(size_t i = 0; i < _widths.size(); ++i)
                {
                    ctx.push_bounds(child_bounds[_widths.size() - 1 - i]);
                }
            }

            void render(NVGcontext* ctx, const rect& bounds)
            {

            }

            std::vector<float> _widths;
            float _height;
        private:
            context& _ctx;
        };

        /// draw elements

        struct elem_debug : public element<elem_debug>
        {
            using element::element;
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
        // imgui::circle_element ce([=](auto& self)
        // {
        //     style_t style = g_ui->peek_style();
        //     style.x = width / 2.0f;
        //     style.y = height / 2.0f;
        //     style.width = width;
        //     style.height = height;
        //     style.fill_color = nvgRGB(255, 128, 0);
        //     self.render(ctx, style);
        //     return style;
        // });

#if 0
        mxCircle(
            style.x = width/2.0f; 
            style.y = height/2.0f; 
            style.width = width/3.0f;
            style.height = height/3.0f;
            style.fill_color = nvgRGB(255, 255, 0););
        mxCircle(
            style.x = width/2.0f;
            style.y = height/2.0f;
            style.width = width/4.0f;
            style.height = height/4.0f;
            style.fill_color = nvgRGB(16,16, 16);
        );

#endif        
        imgui::context imgui_ctx(ctx);
        imgui_ctx.push_bounds({{0.0f, 0.0f},{float(width), float(height)}});

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
            if(imgui::elem_static_row dyn_row(imgui_ctx, [&](auto& self)
            {
                auto& width = self._widths;
                auto& height = self._height;
                width = {25.0f, 20.0f, 50.0f, 100.0f};
                height = 100;
            }); true)
            {
                imgui::elem_debug(imgui_ctx, [](auto& self){});
                imgui::elem_debug(imgui_ctx, [](auto& self){});
                imgui::elem_debug(imgui_ctx, [](auto& self){});
                imgui::elem_debug(imgui_ctx, [](auto& self){});
            }            
        }

        // imgui::elem_dynamic_row row;

#if 0
        mxGroup(style.position = {0,0}; style.size = {width, height}; style.fill = 0xFFFFFF;);
        {
            mxStaticRow(style.size = {width, 100}; style.layout = {50, 100, 100};)
            {
                mxLabel("Hello!");
                mxSpacer();
                mxLabel("World!");
            }
            mxDynamicRow(style.size = {width, 100} style.children = 2;)
            {
                mxLabel("Left");
                mxLabel("Right");
            }
        }
        }
#endif        
    }
}
