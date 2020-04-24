#pragma once

#include "color.hpp"
#include "array.hpp"
#include "stylesheet.hpp"
#include "linear_constraint_solver.hpp"

namespace tabletopify
{
    // immmediate mode gui
    namespace imgui
    {
        template<typename... ARGS>
        class callback
        {
        public:
            ~callback()
            {
                if (delete_func != nullptr)
                {
                    delete_func();
                }
            }

            constexpr callback& operator=(void(*function)(ARGS...))
            {
                exec_func = function;
                delete_func = [](){};
                return *this;
            }

            template<typename FUNCTOR>
            callback& operator=(FUNCTOR&& functor)
            {
                MX_ASSERT(delete_func == nullptr && exec_func == nullptr);
                // convertble to a raw function pointer
                if constexpr (std::is_convertible<FUNCTOR, void(*)(ARGS...)>::value)
                {
                    this->exec_func = static_cast<void(*)(ARGS...)>(functor);
                    this->delete_func = nullptr;
                }
                // functor object
                else
                {
                    // ensure we'r enot overriding a callback somehow
                    MX_ASSERT(singleton<FUNCTOR>::assigned() == false);

                    // move to static storge
                    singleton<FUNCTOR>::move(std::move(functor));

                    this->exec_func = [](ARGS... args) -> void
                    {
                        singleton<FUNCTOR>::get()(std::forward<ARGS>(args)...);
                    };
                    this->delete_func = &singleton<FUNCTOR>::free;
                }

                return *this;
            }

            constexpr void operator()(auto&&... args)
            {
                if (exec_func)
                {
                    exec_func(std::forward<decltype(args)>(args)...);
                }
            }
        private:

            template<typename FUNCTOR>
            constexpr void set_callbacks()
            {
                delete_func = &singleton<FUNCTOR>::free;
                exec_func = [](ARGS... args) -> void
                {
                    singleton<FUNCTOR>::get()(args...);
                };
            }

            void(*delete_func)() = nullptr;
            void(*exec_func)(ARGS...) = nullptr;
        };

        struct widget_base;

        typedef rect (*bounds_callback_t)(widget_base*, const rect&);

        struct render_bounds
        {
            bounds_callback_t callback = nullptr;
            widget_base* widget = nullptr;

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

            constexpr const style_sheet& style() const {return *_style;}

            // common funcs
            void resolve_box_layout_impl(element_type_t et, widget_flag_t flags);
            void resolve_scissor_impl();
            element_state_t resolve_element_state_impl();
            void resolve_style_impl(element_type_t, element_state_t);
            void render_box_layout_impl() const;

            // widget interface methods
            static rect bounds_callback(widget_base* wb, const rect& desired) {  MX_ASSERT(!"Empty bounds callback, widget should not have children"); return {};}
            void first_update_impl() {}
            void middle_update_impl() {}
            void first_render_impl() const {}
            void last_update_impl() {}
            void last_render_impl() const {}

        private:
            // shared margin/border/padding rendering
            // returns updated bounds
            rect render_border(const rect& bounds) const;

            const style_sheet* _style = nullptr;
        public:
            void requestSize(const vec2& size);
            void requestWidth(double width);
            void requestHeight(double height);
            void requestPosition(const vec2& position);
            void requestBounds(const rect& bounds);

            constexpr rect scissor() const {return _scissor; }
            constexpr rect childScissor() const {return _childScissor; }

            array<utf8_string> tags;

        protected:

            box_layout _layout = {};
            rect _scissor = {};
            rect _childScissor = {};
            rect _desiredBounds = {};
            bool _desiredWidthRequested = false;
            bool _desiredHeightRequested = false;
            widget_data* _data = nullptr;
            size_t _childCounter = 0;
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
                if constexpr (WIDGET_FLAGS & widget_flag::scissors) {
                    g_context.pop_scissor();
                }

                auto* self = static_cast<T*>(this);
                self->last_update();
                self->last_render();

                g_context.pop_render_bounds();
            }

            void resolve_box_layout()
            {
                auto* self = static_cast<T*>(this);
                self->resolve_box_layout_impl(ELEMENT_TYPE, WIDGET_FLAGS);
                self->resolve_scissor_impl();

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

            void middle_update()
            {
                static_cast<T*>(this)->middle_update_impl();
            }

            void push_render_bounds()
            {
                auto* self = static_cast<T*>(this);
                render_bounds rb = {&T::bounds_callback, self};
                g_context.push_render_bounds(rb);
                if constexpr (WIDGET_FLAGS & widget_flag::scissors) {
                    g_context.push_scissor(this->childScissor());
                }
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

        //
        // Layout Widgets
        //

        // scroll_area

        namespace scroll_direction
        {
            enum values
            {
                none       = 0x0,
                horizontal = 0x1,
                vertical   = 0x2,
                both       = horizontal | vertical,
            };
        }
        typedef scroll_direction::values scroll_direction_t;

        struct scroll_area_data : public widget_data
        {
            scroll_direction_t direction = scroll_direction::none;
            struct
            {
                double scroll_offset = 0.0;
                double grab_offset = 0.0;
                bool grabbing = false;
            } horizontal, vertical;
        };

        struct widget_scroll_area : public widget<widget_scroll_area, element_type::scroll_area, scroll_area_data, widget_flag::scissors>
        {
        public:
            using widget::widget;

            void resolve_scissor_impl();
            void first_update_impl();
            static rect bounds_callback(widget_base* parent, const rect& desired);
            void last_update_impl();
            void last_render_impl() const;
        private:
            struct
            {
                // cumulative size of the child widgets
                double internal_size = 0.0;
                // area of entire scrollbar element
                rect scrollbar_area = {};
                // area of 'grabable' scrollbar element
                rect scrollbar_grab_area = {};
            } horizontal, vertical;
        };

        // flex box

        namespace layout_direction
        {
            enum values
            {
                none       = 0x0,
                horizontal = 0x1,
                vertical   = 0x2,
            };
        }
        typedef layout_direction::values layout_direction_t;

        struct flex_box_data : public widget_data
        {
            linear_constraint_solver constraints;

            flex_box_data() = default;
            flex_box_data(const std::initializer_list<constraint>& constraintList) : constraints(constraintList) { }
        };

        struct widget_flex_box : public widget<widget_flex_box, element_type::flex_box, flex_box_data, widget_flag::scissors>
        {
            using widget::widget;

            void resolve_box_layout_impl(element_type_t et, widget_flag_t flags);
            static rect bounds_callback(widget_base* parent, const rect& desired);

            layout_direction_t direction;
        private:
            size_t _index = 0;
            double _offset = 0.0;
        };

        // spacer

        struct widget_spacer : public widget<widget_spacer, element_type::spacer>
        {
            using widget::widget;
        };

        //
        // Interaction Widgets
        //

        // label

        struct label_data : widget_data
        {
            utf8_string text;
            label_data() = default;
            label_data(utf8_string&& text) : text(std::move(text)) {}
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
            button_data() = default;
            button_data(utf8_string text) : text(std::move(text)) {}
        };

        struct widget_button : public widget<widget_button, element_type::button, button_data, widget_flag::scissors>
        {
        public:
            using widget::widget;

            void first_update_impl();
            void first_render_impl() const;
            void last_update_impl();

            callback<> on_click;
        private:
            bool _clicked = false;
        };

        /// text box

        typedef enum class text_box_action
        {
            empty,
            write_whitespace,
            write_nonwhitespace,
            insert_block,
            erase_block,
            erase_left,
            erase_right,
            redo,
            undo,
        } text_box_action_t;

        struct text_box_data : widget_data
        {
            utf8_stringbuffer sb;
            utf8_string text;
            utf8_string placeholderText;
            double textOffset = 0.0;

            size_t selectBegin = 0;
            size_t selectEnd = 0;
            text_box_action_t previousAction = text_box_action::empty;
            struct snapshot
            {
                utf8_string text;
                size_t selectBegin;
                size_t selectEnd;
            };
            std::vector<snapshot> undoStack;
            std::vector<snapshot> redoStack;
            text_box_data() = default;
            text_box_data(utf8_string text, utf8_string placeholderText)
            : sb(text)
            , text(std::move(text))
            , placeholderText(std::move(placeholderText))
            {}
        };

        struct widget_text_box : public widget<widget_text_box, element_type::text_box, text_box_data>
        {
        public:
            using widget::widget;

            void middle_update_impl();
            void first_render_impl() const;

            callback<const utf8_string&> on_submit;
        };

        // slider

        struct slider_data : widget_data
        {
            double minVal;
            double maxVal;
            double currentVal;

            slider_data(double minVal, double maxVal, double currentVal)
            : minVal(minVal)
            , maxVal(maxVal)
            , currentVal(currentVal)
            { }
        };

        struct widget_slider : public widget<widget_slider, element_type::slider, slider_data>
        {
        public:
            using widget::widget;

            static rect bounds_callback(widget_base* parent, const rect& desired);
            void last_update_impl();

            callback<double> on_change;
        private:
            vec2 handle_center(double width) const;
            friend struct widget_slider_handle;

        };

        // checkbox

        struct checkbox_data : widget_data
        {
            bool checked;
            checkbox_data(bool checked)
            : checked(checked)
            { }
        };

        struct widget_checkbox : public widget<widget_checkbox, element_type::checkbox, checkbox_data>
        {
        public:
            using widget::widget;

            element_state_t resolve_element_state_impl();
            void first_update_impl();
            void last_update_impl();

            callback<bool> on_change;
        private:
            bool _clicked;

        };
    } // imgui namespace
}

// user logic
// resolve collision geometry
// trigger input/collision callbacks
// determine style selector state (hover,active,etc)
// render borders/margin/etc
// widget-specific first render
//  child widgets
// widget-specific second render

#define MX_WIDGET_CODEGEN(X, PDATA, ...)\
    if (imgui::X widget##__LINE__(PDATA); \
        [&](auto& self) -> bool {\
            __VA_ARGS__\
            ;\
            self.resolve_box_layout(); /* for click detection */\
            self.first_update(); /* post geometry but pre style */\
            self.resolve_style(); /* get the style */\
            self.middle_update(); /* post style resolution but pre render */\
            self.render_box_layout(); /* common box layout rendering */\
            self.first_render(); /* widget specific rendering */\
            self.push_render_bounds();\
            return true;\
        }(widget##__LINE__))

#define mxRoot(VIEWPORT) MX_WIDGET_CODEGEN(widget_root, VIEWPORT)
#define mxScrollArea(DATA, ...) MX_WIDGET_CODEGEN(widget_scroll_area, DATA, __VA_ARGS__)
#define mxHScroll(DATA, ...) MX_WIDGET_CODEGEN(widget_scroll_area, DATA, self.data()->direction = imgui::scroll_direction::horizontal; __VA_ARGS__)
#define mxVScroll(DATA, ...) MX_WIDGET_CODEGEN(widget_scroll_area, DATA, self.data()->direction = imgui::scroll_direction::vertical; __VA_ARGS__)
#define mxFlexBox(DATA, ...) MX_WIDGET_CODEGEN(widget_flex_box, DATA, __VA_ARGS__)
#define mxHBox(DATA, ...) MX_WIDGET_CODEGEN(widget_flex_box, DATA, self.direction = imgui::layout_direction::horizontal; __VA_ARGS__)
#define mxVBox(DATA, ...) MX_WIDGET_CODEGEN(widget_flex_box, DATA, self.direction = imgui::layout_direction::vertical; __VA_ARGS__)
#define mxSpacer(...) MX_WIDGET_CODEGEN(widget_spacer, nullptr, __VA_ARGS__)
#define mxLabel(DATA, ...) MX_WIDGET_CODEGEN(widget_label, DATA, __VA_ARGS__)
#define mxButton(DATA, ...) MX_WIDGET_CODEGEN(widget_button, DATA, __VA_ARGS__)
#define mxTextBox(DATA, ...) MX_WIDGET_CODEGEN(widget_text_box, DATA, __VA_ARGS__)
#define mxSlider(DATA, ...) MX_WIDGET_CODEGEN(widget_slider, DATA, __VA_ARGS__)
#define mxCheckbox(DATA, ...) MX_WIDGET_CODEGEN(widget_checkbox, DATA, __VA_ARGS__)
