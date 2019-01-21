#pragma once

namespace tabletopify
{
    // immmediate mode gui
    namespace imgui
    {
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
                singleton<FUNCTOR>::construct(std::move(functor));
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

            void(*delete_func)() = [](){};
            RETVAL(*exec_func)(ARGS...) = [](ARGS...) -> RETVAL {return RETVAL();};
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
            size_t _idx = 0; Vagrant Story
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

        typedef rect (*render_bounds_callback_t)(void*, const rect&);

        struct render_bounds
        {
            render_bounds_callback_t callback = nullptr;
            void* callback_data = nullptr;
            rect scissor = {};

            rect eval(const rect& desired) const;
        };

        typedef size_t widget_id_t;
        widget_id_t next_widget_id();

        class context
        {
        public:
            ~context()
            {
                MX_ASSERT(_render_bounds_stack.size() == 0);
            }

            void push_render_bounds(const render_bounds&);
            const render_bounds& peek_render_bounds() const;
            void pop_render_bounds();
        private:
            std::vector<render_bounds> _render_bounds_stack;
            widget_id_t _focused_widget = 0;
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

            // destructor called after child widgets have been handled
            ~element()
            {
                // restore the scissor area for second render pass
                auto scissor = _ctx.peek_render_bounds().scissor;
                get_nanovg()->set_scissor(scissor);

                auto& self = *static_cast<T*>(this);
                // first update and render after child renders
                self.middle();
                self.second_render();
                // our final update
                self.end();

                _ctx.pop_render_bounds();

            }

            // gets and saves this widget's render bounds and scissor area
            // and pushes callback and current scissor for child widgets
            void calc_render_bounds()
            {
                const auto& rb = _ctx.peek_render_bounds();

                bounds = rb.eval(bounds);
                const auto& parent_scissor = rb.scissor;

                // calc the scissor geometry
                auto scissor = get_intersecting_rect(parent_scissor, bounds);
                get_nanovg()->set_scissor(scissor);

                // push info for child render bounds calculations
                auto& self = *static_cast<T*>(this);
                _ctx.push_render_bounds({self.get_bounds_callback(), this, scissor});
            }

            rect bounds;
        protected:
            context& _ctx;
        };

        struct group_data
        {
            widget_id_t id = next_widget_id();
            // vertical offset down 
            double vertical_scroll = 0.0;
            // is user grabbng scrollbar with mouse
            bool grabbing_scrollbar = false;
            // offset into scrollbar handle user is grabbng
            double grab_offset_y = 0.0;
        };

        struct elem_group : public element<elem_group>
        {
            using element::element;

            void begin();
            void first_render() const {}
            render_bounds_callback_t get_bounds_callback();
            void middle();
            void second_render() const;
            void end();

            group_data* data = nullptr;
        private:
            // cumulative height of the child elements
            double internal_height = 0.0;
            // area of entire scrollbar widget
            rect scrollbar_area = {};
            // grab area of scrollbar this frame
            rect scrollbar_grab_area = {};
            constexpr static double scrollbar_width = 20.0;
            constexpr static double min_scrollbar_height = 150.0;            
            constexpr static double scroll_step = 30.0;
        };

        struct elem_static_row : public element<elem_static_row>
        {
            using element::element;

            void begin() {}
            void first_render() const {}
            render_bounds_callback_t get_bounds_callback();
            void middle() {}
            void second_render() const {}
            void end() {}

            std::vector<float> _widths;
            float _horizontal_offset = 0.0f;            
        };

        struct elem_debug : public element<elem_debug>
        {
            using element::element;

            void begin();
            void first_render() const;
            render_bounds_callback_t get_bounds_callback();
            void middle() {}
            void second_render() const {}
            void end() {}

            callback<void, vec2> on_mouse_move;
            callback<void, vec2> on_mouse_over;
            NVGcolor bg_color = nvgRGB(0, 0xff, 0xff);
        };
    } // imgui namespace


#define MX_ELEMENT_CODEGEN(X, ...)\
    if (imgui::X group##__LINE__(imgui_ctx); \
        [&](auto& self) -> bool {\
            __VA_ARGS__\
            self.calc_render_bounds();\
            self.begin();\
            self.first_render();\
            return true;\
        }(group##__LINE__))

#define mxGroup(...) MX_ELEMENT_CODEGEN(elem_group, __VA_ARGS__)
#define mxStaticRow(...) MX_ELEMENT_CODEGEN(elem_static_row, __VA_ARGS__)
#define mxDebug(...) MX_ELEMENT_CODEGEN(elem_debug, __VA_ARGS__)

}