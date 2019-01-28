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
        constexpr widget_id_t NULL_WIDGET_ID = 0;
        constexpr widget_id_t INVALID_WIDGET_ID = ~NULL_WIDGET_ID;
        widget_id_t next_widget_id();

        struct widget_data
        {
            widget_id_t id = next_widget_id();
        };

        class context
        {
        public:
            void update(double dt);

            // render bound tree
            void push_render_bounds(const render_bounds&);
            const render_bounds& peek_render_bounds() const;
            void pop_render_bounds();

            // widget focus methods
            constexpr void focus_widget(widget_id_t id) { _focused_widget = id; }
            constexpr void unfocus_widget(widget_id_t id) { if (is_focused(id)) { focus_widget(INVALID_WIDGET_ID); } }
            constexpr bool is_focused(widget_id_t id)  const { return id == _focused_widget; }
        private:
            std::vector<render_bounds> _render_bounds_stack;
            widget_id_t _focused_widget = INVALID_WIDGET_ID;
        };

        // base element

        template<typename T, typename WIDGET_DATA=widget_data>
        struct element
        {
            element() = delete;
            element(const element&) = delete;
            element(element&&) = delete;

            constexpr element(context& ctx)
            : data(nullptr)
            , _ctx(ctx)
            { }

            // destructor called after child widgets have been handled
            ~element()
            {
                // restore the scissor area for second render pass
                scissor = _ctx.peek_render_bounds().scissor;
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
                scissor = get_intersecting_rect(parent_scissor, bounds);
                get_nanovg()->set_scissor(scissor);

                // push info for child render bounds calculations
                auto& self = *static_cast<T*>(this);
                _ctx.push_render_bounds({self.get_bounds_callback(), this, scissor});
            }

            // widget methods

            constexpr widget_id_t get_widget_id() const
            {
                return data ? data->id : NULL_WIDGET_ID;
            }

            constexpr void focus_widget()
            {
                _ctx.focus_widget(get_widget_id());
            }

            constexpr void unfocus_widget()
            {
                _ctx.unfocus_widget(get_widget_id());
            }

            constexpr bool is_focused() const
            {
                return _ctx.is_focused(get_widget_id());
            }

            rect bounds;
            rect scissor;

            WIDGET_DATA* data;
        private:
            context& _ctx;
        };

        struct group_data : widget_data
        {
            // vertical offset down 
            double vertical_scroll = 0.0;
            // is user grabbng scrollbar with mouse
            bool grabbing_scrollbar = false;
            // offset into scrollbar handle user is grabbng
            double grab_offset_y = 0.0;
        };

        struct elem_group : public element<elem_group, group_data>
        {
            using element::element;

            void begin();
            void first_render() const {}
            render_bounds_callback_t get_bounds_callback();
            void middle();
            void second_render() const;
            void end();

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