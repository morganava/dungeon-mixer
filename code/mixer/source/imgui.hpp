#pragma once

#include "color.hpp"
#include "array.hpp"
#include "stylesheet.hpp"

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
                return callback(widget, desired);
            }

            rect (*callback)(void*, const rect&) = [](void*, const rect&) -> rect {MX_ASSERT(!"Should Not Be Called"); return {};};
            void* widget = nullptr;
        };

        typedef rect (*render_bounds_callback_t)(void*, const rect&);

        struct render_bounds
        {
            render_bounds_callback_t callback = nullptr;
            void* callback_data = nullptr;
            rect scissor = {};

            rect eval(const rect& desired) const;
        };

        // widget id

        typedef size_t widget_id_t;
        constexpr widget_id_t NULL_WIDGET_ID = 0;
        constexpr widget_id_t INVALID_WIDGET_ID = ~NULL_WIDGET_ID;
        widget_id_t next_widget_id();

        // context

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

            // set's render scissor
            void set_render_scissor(const rect&);

        private:
            std::vector<render_bounds> _render_bounds_stack;
            widget_id_t _focused_widget = INVALID_WIDGET_ID;
        };

        inline void init_context()
        {
            singleton<context>::construct();
        }
        constexpr void free_context()
        {
            singleton<context>::free();
        }
        inline constexpr context& g_context = singleton<context>::get();

        struct widget_data
        {
            widget_id_t id = next_widget_id();
        };

        // base widget

        struct widget_base
        {
        public:
            widget_base(widget_data* data);

            constexpr widget_id_t get_widget_id() const
            {
                return _data ? _data->id : NULL_WIDGET_ID;
            }

        protected:
            void focus_widget();
            void unfocus_widget();
            bool is_focused() const;

            const style_sheet& style() const {return *_style;}

            // common funcs
            void resolve_box_layout_impl(element_type_t et);
            void resolve_style_impl(element_type_t);

            // widget interface methods
            void first_update_impl() {}
            void first_render_impl() const {}
            void last_update_impl() {}
            void last_render_impl() const {}

        private:
            // shared margin/border/padding rendering
            // returns updated bounds
            rect render_border(const rect& bounds) const;

            const style_sheet* _style = nullptr;
        public:
            rect bounds = {};
            rect scissor = {};
            box_layout layout = {};
            array<utf8_string> tags;

        protected:

            widget_data* _data = nullptr;
        };

        template<typename T, element_type ELEMENT_TYPE, typename WIDGET_DATA=widget_data>
        struct widget : public widget_base
        {
            widget() = delete;
            widget(const widget&) = delete;
            widget(widget&&) = delete;

            constexpr widget(widget_data* pdata)
            : widget_base(pdata)
            { }

            // destructor called after child widgets have been handled
            ~widget()
            {
                auto* self = static_cast<T*>(this);
                // first update and render after child renders
                // self->last_update();
                // self->second_render();
                // // our final update
                // self->end();
                self->last_update_impl();
                self->last_render_impl();

                g_context.pop_render_bounds();
            }

            // gets and saves this widget's render bounds and scissor area
            // and pushes callback and current scissor for child widgets
            void calc_render_bounds()
            {
                calc_render_bounds_impl([](void* pSelf) -> render_bounds
                {
                    auto* self = static_cast<T*>(pSelf);
                    return {self->get_bounds_callback(), self, self->scissor};
                });
            }

            void resolve_box_layout()
            {
                this->resolve_box_layout_impl(ELEMENT_TYPE);
            }

            // resolve this widget's style
            void resolve_style()
            {
                this->resolve_style_impl(ELEMENT_TYPE);
            }

            void first_update()
            {
                static_cast<T*>(this)->first_update_impl();
            }

            void first_render() const
            {
                static_cast<const T*>(this)->first_render_impl();
            }

            void push_render_bounds()
            {
                auto* self = static_cast<T*>(this);
                render_bounds rb = {self->get_bounds_callback(), self, self->scissor};
                g_context.push_render_bounds(rb);
            }

            void last_update()
            {
                static_cast<T*>(this)->last_update_impl();
            }

            void last_render() const
            {
                static_cast<const T*>(this)->last_render_impl();
            }

            // widget methods

            constexpr WIDGET_DATA* data()
            {
                return reinterpret_cast<WIDGET_DATA*>(this->_data);
            }

            constexpr const WIDGET_DATA* data() const
            {
                return reinterpret_cast<const WIDGET_DATA*>(this->_data);
            }
        };

        // grouping widgets

        struct vbox_data : widget_data
        {
            // vertical offset down
            double vertical_scroll = 0.0;
            // is user grabbing scrollbar with mouse
            bool grabbing_scrollbar = false;
            // offset into scrollbar handle user is grabbng
            double grab_offset_y = 0.0;
        };

        struct widget_vbox : public widget<widget_vbox, element_type::vbox, vbox_data>
        {
            using widget::widget;

            void first_update_impl();
            render_bounds_callback_t get_bounds_callback();
            void last_update_impl();
            void last_render_impl() const;

        private:
            // cumulative height of the child widgets
            double internal_height = 0.0;
            // area of entire scrollbar widget
            rect scrollbar_area = {};
            // grab area of scrollbar this frame
            rect scrollbar_grab_area = {};
        };

        struct widget_hbox : public widget<widget_hbox, element_type::hbox>
        {
            using widget::widget;

            render_bounds_callback_t get_bounds_callback();

            array_view<double> widths;
        private:
            size_t _index = 0;
            float _horizontal_offset = 0.0f;
        };

        // debug renderer

        struct widget_debug : public widget<widget_debug, element_type::debug>
        {
            using widget::widget;

            void first_update_impl();
            void first_render_impl() const;
            render_bounds_callback_t get_bounds_callback() { return nullptr; }

            callback<void, vec2> on_mouse_move;
            callback<void, vec2> on_mouse_over;
            color bg_color = color::rgb(0,0,0);
        };

        // label

        struct label_data : widget_data
        {
            utf8_string text;
        };

        struct widget_label : public widget<widget_label, element_type::label, label_data>
        {
            using widget::widget;

            void first_render_impl() const;
            render_bounds_callback_t get_bounds_callback() { return nullptr; }
        };

        /// button

        struct button_data : widget_data
        {
            utf8_string text;
        };

        struct widget_button : public widget<widget_button, element_type::button, button_data>
        {
            using widget::widget;

            void first_update_impl();
            void first_render_impl() const;
            render_bounds_callback_t get_bounds_callback() { return nullptr; }
            void last_update_impl();

            callback<void> on_click;
        };

    } // imgui namespace


// user logic
// resolve collision geometry
// trigger input/collision callbacks
// determine style selector state (hover,active,etc)
// render borders/margin/etc
// widget-specific first render
//  child widgets
// widget-specific second render

#define MX_WIDGET_CODEGEN(X, pdata, ...)\
    if (imgui::X widget##__LINE__(pdata); \
        [&](auto& self) -> bool {\
            __VA_ARGS__\
            self.resolve_box_layout();\
            self.first_update();\
            self.first_render();\
            self.push_render_bounds();\
            return true;\
        }(widget##__LINE__))

#define mxVbox(data, ...) MX_WIDGET_CODEGEN(widget_vbox, data, __VA_ARGS__)
#define mxHbox(...) MX_WIDGET_CODEGEN(widget_hbox, nullptr, __VA_ARGS__)
#define mxDebug(...) MX_WIDGET_CODEGEN(widget_debug, nullptr, __VA_ARGS__)
#define mxLabel(data, ...) MX_WIDGET_CODEGEN(widget_label, data, __VA_ARGS__)
#define mxButton(data, ...) MX_WIDGET_CODEGEN(widget_button, data, __VA_ARGS__)

}