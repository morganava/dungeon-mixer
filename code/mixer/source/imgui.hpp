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

        struct widget_base;

        typedef rect (*bounds_callback_t)(widget_base*, const rect&);

        struct render_bounds
        {
            bounds_callback_t callback = nullptr;
            widget_base* widget = nullptr;
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
            void push_scissor(const rect&);
            void pop_scissor();


        private:
            std::vector<render_bounds> _render_bounds_stack;
            std::vector<rect> _render_scissor_stack;
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

        namespace widget_flag
        {
            enum values
            {
                none        = 0x0,
                scissors    = 0x1,
            };
        }
        typedef widget_flag::values widget_flag_t;

        // widget base-class

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
            void resolve_box_layout_impl(element_type_t et, widget_flag_t flags);
            element_state_t resolve_element_state_impl() const {return element_state::none;}
            void resolve_style_impl(element_type_t, element_state_t);
            void render_box_layout_impl() const;

            // widget interface methods
            static rect bounds_callback(widget_base* wb, const rect& desired) {  MX_ASSERT(!"Empty bounds callback, widget should not have children"); return {};}
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
            /* TODO: kill bounds, use layout instead */
            rect bounds = {};
            rect scissor = {};
            box_layout layout = {};
            array<utf8_string> tags;

        protected:

            widget_data* _data = nullptr;
        };

        template<typename T, element_type ELEMENT_TYPE, typename WIDGET_DATA=widget_data, widget_flag_t WIDGET_FLAGS=widget_flag::none>
        struct widget : public widget_base
        {
        protected:
            widget() : widget(nullptr) {};
        public:
            widget(const widget&) = delete;
            widget(widget&&) = delete;

            constexpr widget(widget_data* pdata)
            : widget_base(pdata)
            { }

            // destructor called after child widgets have been handled
            ~widget()
            {
                auto* self = static_cast<T*>(this);
                self->last_update_impl();
                self->last_render_impl();

                if (WIDGET_FLAGS & widget_flag::scissors) {
                    g_context.pop_scissor();
                }

                g_context.pop_render_bounds();
            }

            void resolve_box_layout()
            {
                this->resolve_box_layout_impl(ELEMENT_TYPE, WIDGET_FLAGS);
            }

            void render_box_layout() const
            {
                static_cast<const T*>(this)->render_box_layout_impl();
            }

            // resolve this widget's style
            void resolve_style()
            {
                auto self = static_cast<T*>(this);
                self->resolve_style_impl(ELEMENT_TYPE, self->resolve_element_state_impl());
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
                render_bounds rb = {&T::bounds_callback, self, self->scissor};
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

        // the root widget with viewport info
        struct widget_root : public widget<widget_root, element_type::root, widget_data, widget_flag::scissors>
        {
            using widget::widget;
            widget_root(const rect& viewport);
            ~widget_root();

            static rect bounds_callback(widget_base* parent, const rect& desired);
            static rect root_bounds_callback(widget_base* parent, const rect& desired);
        private:
            rect _viewport;
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

        struct widget_vbox : public widget<widget_vbox, element_type::vbox, vbox_data, widget_flag::scissors>
        {
            using widget::widget;

            void first_update_impl();
            static rect bounds_callback(widget_base* parent, const rect& desired);
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

            static rect bounds_callback(widget_base* parent, const rect& desired);

            array_view<double> widths;
        private:
            size_t _index = 0;
            float _horizontal_offset = 0.0f;
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
        };

        /// button

        struct button_data : widget_data
        {
            utf8_string text;
        };

        struct widget_button : public widget<widget_button, element_type::button, button_data>
        {
            using widget::widget;

            element_state_t resolve_element_state_impl() const;
            void first_update_impl();
            void first_render_impl() const;
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
            self.resolve_style();\
            self.render_box_layout();\
            self.first_render();\
            self.push_render_bounds();\
            return true;\
        }(widget##__LINE__))

#define mxRoot(viewport, ...) MX_WIDGET_CODEGEN(widget_root, viewport, __VA_ARGS__)
#define mxVbox(data, ...) MX_WIDGET_CODEGEN(widget_vbox, data, __VA_ARGS__)
#define mxHbox(...) MX_WIDGET_CODEGEN(widget_hbox, nullptr, __VA_ARGS__)
#define mxDebug(...) MX_WIDGET_CODEGEN(widget_debug, nullptr, __VA_ARGS__)
#define mxLabel(data, ...) MX_WIDGET_CODEGEN(widget_label, data, __VA_ARGS__)
#define mxButton(data, ...) MX_WIDGET_CODEGEN(widget_button, data, __VA_ARGS__)

}