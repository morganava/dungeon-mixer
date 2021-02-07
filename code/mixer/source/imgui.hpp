#pragma once

#include "color.hpp"
#include "array.hpp"
#include "stylesheet.hpp"
#include "enums.hpp"
#include "callback.hpp"

namespace dm
{
    // immmediate mode gui
    namespace imgui
    {
        MX_MAKE_ENUM(render_pass, uint_fast8_t,
            geometry, /* only calculates layout information */
            full /* layout, input, render */
        );

        // widget id
        MX_MAKE_HANDLE_ENUM(widget_id,
            null
        );

        // forward declare for context class
        struct widget_base;

        // context
        class context
        {
        public:
            void update(double dt);

            void push_widget(widget_base*);
            void pop_widget();
            widget_base* parent_widget() const;

            // root widget focus methods for 'window' like widgets
            void focus_window_widget(widget_id_t);
            void unfocus_window_widget(widget_id_t);
            bool is_window_widget_focused(widget_id_t) const;
            void set_current_window_widget(widget_id_t);
            bool is_current_window_widget_focused() const;

            // widget focus methods
            constexpr void focus_widget(widget_id_t id) { _focused_widget = id; }
            constexpr void unfocus_widget(widget_id_t id) { if (is_focused(id)) { focus_widget(widget_id::invalid); } }
            constexpr bool is_focused(widget_id_t id)  const { return id == _focused_widget; }

            // set's render scissor
            void push_scissor(const rect&);
            void pop_scissor();

            render_pass_t get_render_pass() const {
                return _currentRenderPass;
            }

            void set_render_pass(render_pass_t renderPass)
            {
                this->_currentRenderPass = renderPass;
            }

        private:
            render_pass_t _currentRenderPass = render_pass::full;

            std::vector<widget_base*> _widget_stack;
            std::vector<rect> _render_scissor_stack;
            widget_id_t _focused_window_widget = widget_id::invalid;
            widget_id_t _focused_widget = widget_id::invalid;
            widget_id_t _current_window_widget = widget_id::invalid;
        };
        MX_MAKE_SINGLETON(context);

        struct widget_data
        {
            widget_id_t id = next_widget_id();
        };

        MX_MAKE_FLAG_ENUM(widget_flag, uint32_t,
            has_children = MX_FLAG(0),
            scissors     = MX_FLAG(1),
            windows      = MX_FLAG(2)
        );

        // widget base-class

        struct widget_base
        {
        protected:
            constexpr const style_sheet& style() const {return *_style;}

            // common funcs
            virtual rect bounds_callback(rect desiredBounds) = 0;

            void resolve_box_layout_impl(element_type_t et, widget_flag_t flags);
            void resolve_scissor_impl();
            element_state_t resolve_element_state_impl();
            void resolve_style_impl(element_type_t, element_state_t);
            void render_box_layout_impl() const;

            // widget interface methods
            rect bounds_callback_impl(rect desired);
            constexpr bool should_draw_impl() const {return true;}
            void handle_input_impl() {}
            void first_update_impl() {}
            void middle_update_impl() {}
            void first_render_impl() const {}
            void last_update_impl() {}
            void last_render_impl() const {}
        public:
            void requestSize(const vec2& size);
            void requestWidth(double width);
            void requestHeight(double height);
            void requestPosition(const vec2& position);
            void requestBounds(const rect& bounds);

            constexpr rect scissor() const {return _scissor; }
            constexpr rect childScissor() const {return _childScissor; }

            array<utf8_string> tags;

            void increment_child_count() { _childCounter++; }

        protected:
            const style_sheet* _style = nullptr;
            box_layout _layout = {};
            rect _scissor = {};
            rect _childScissor = {};
            rect _desiredBounds = {};
            bool _desiredWidthRequested = false;
            bool _desiredHeightRequested = false;
            size_t _childCounter = 0;
        };

        template<typename T, element_type ELEMENT_TYPE, typename WIDGET_DATA=widget_data, widget_flag_t WIDGET_FLAGS=widget_flag::none>
        struct widget : public widget_base
        {
        public:
            widget(const widget&) = delete;
            widget(widget&&) = delete;

            constexpr widget(WIDGET_DATA* pdata)
            : _widget_data(std::nullopt)
            , _data(pdata)
            { }

            constexpr widget(WIDGET_DATA&& rdata)
            : _widget_data(std::make_optional(std::move(rdata)))
            , _data(&_widget_data.value())
            { }

            // destructor called after child widgets have been handled
            ~widget()
            {
                auto& self = static_cast<T&>(*this);

                if (self.should_draw())
                {
                    if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::scissors)) {
                        g_context.pop_scissor();
                    }

                    if (g_context.get_render_pass() == render_pass::full)
                    {

                        self.last_update();
                        self.last_render();
                    }

                    if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::has_children))
                    {
                        g_context.pop_widget();
                    }

                    if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::windows))
                    {
                        g_context.set_current_window_widget(widget_id::invalid);
                    }
                }
            }

            // internalish draw methods
            bool draw(render_pass_t renderPass, auto&& lambda) {

                auto& self = static_cast<T&>(*this);
                if (!self.should_draw()) {
                    return false;
                }

                if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::windows)) {
                    g_context.set_current_window_widget(this->get_widget_id());
                }

                self._childCounter = 0;
                g_context.set_render_pass(renderPass);

                lambda(self);

                switch(renderPass)
                {
                    case render_pass::geometry:
                        self.resolve_box_layout(); /* for click detection */
                        self.push_widget(); /* pushes widget and scissor */
                        return true;
                    case render_pass::full:
                        self.resolve_box_layout(); /* for click detection */
                        self.handle_input_impl();
                        self.first_update(); /* post geometry but pre style */
                        self.resolve_style(); /* get the style */
                        self.middle_update(); /* post style resolution but pre render */
                        self.render_box_layout(); /* common box layout rendering */
                        self.first_render(); /* widget specific rendering */
                        self.push_widget();
                        return true;
                    default:
                        MX_FAIL_FAST();
                }
                return false;
            }

            constexpr widget_id_t get_widget_id() const
            {
                return _data ? _data->id : widget_id::null;
            }

            void focus_widget()
            {
                g_context.focus_widget(this->get_widget_id());
            }

            void unfocus_widget()
            {
                g_context.unfocus_widget(this->get_widget_id());
            }

            bool is_focused() const
            {
                return g_context.is_focused(this->get_widget_id());
            }

        protected:
            // determines whether draw() should be a noop
            bool should_draw() const
            {
                const auto self = static_cast<const T*>(this);
                return self->should_draw_impl();
            }

            // callback for child widgets to get their draw area
            rect bounds_callback(rect desiredBounds) override
            {
                auto self = static_cast<T*>(this);
                return self->bounds_callback_impl(desiredBounds);
            }

            // calculates the physical dimensions of the widget and the scissor areas
            void resolve_box_layout()
            {
                auto self = static_cast<T*>(this);
                self->resolve_box_layout_impl(ELEMENT_TYPE, WIDGET_FLAGS);
                self->resolve_scissor_impl();
            }

            void handle_input()
            {
                static_cast<T*>(this)->handle_input_impl();
            }

            void first_update()
            {
                static_cast<T*>(this)->first_update_impl();
            }

            // resolve this widget's style
            void resolve_style()
            {
                auto self = static_cast<T*>(this);
                self->resolve_style_impl(ELEMENT_TYPE, self->resolve_element_state_impl());
            }

            void middle_update()
            {
                static_cast<T*>(this)->middle_update_impl();
            }

            // render's the box layout determined bdy the style
            void render_box_layout() const
            {
                static_cast<const T*>(this)->render_box_layout_impl();
            }

            // widget specific rendering
            void first_render() const
            {
                static_cast<const T*>(this)->first_render_impl();
            }

            // pushes widget and scissor to stack if it has children
            void push_widget()
            {
                if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::has_children))
                {
                    g_context.push_widget(this);
                }

                if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::scissors))
                {
                    g_context.push_scissor(this->childScissor());
                }
            }

            // last update in destructor after children
            void last_update()
            {
                static_cast<T*>(this)->last_update_impl();
            }

            // last render in destructor after children
            void last_render() const
            {
                static_cast<const T*>(this)->last_render_impl();
            }

            // widget methods
        protected:
            constexpr WIDGET_DATA* data()
            {
                return reinterpret_cast<WIDGET_DATA*>(this->_data);
            }

            constexpr const WIDGET_DATA* data() const
            {
                return reinterpret_cast<const WIDGET_DATA*>(this->_data);
            }
        private:
            std::optional<WIDGET_DATA> _widget_data = std::nullopt;
            widget_data* const _data = nullptr;
        };

        // the root widget with viewport info
        struct widget_root : public widget<widget_root, element_type::root, widget_data, widget_flag::has_children | widget_flag::scissors | widget_flag::windows>
        {
            using widget::widget;
            widget_root(const rect& viewport);
            ~widget_root();

            void resolve_box_layout_impl(element_type_t et, widget_flag_t flag);
            void render_box_layout_impl() const;
            void resolve_scissor_impl();

            rect bounds_callback_impl(rect desiredBounds);
        private:
            rect _viewport;
        };

        //
        // Layout Widgets
        //

        // scroll_area
        MX_MAKE_FLAG_ENUM(scroll_direction, uint_fast8_t,
            horizontal = MX_FLAG(0),
            vertical   = MX_FLAG(1),
            both       = horizontal | vertical
        );

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

        struct widget_scroll_area : public widget<widget_scroll_area, element_type::scroll_area, scroll_area_data, widget_flag::has_children | widget_flag::scissors>
        {
        public:
            using widget::widget;

            void resolve_scissor_impl();
            void first_update_impl();
            rect bounds_callback_impl(rect desiredBounds);
            void last_update_impl();
            void last_render_impl() const;

            scroll_direction_t direction = scroll_direction::none;
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

        struct widget_flex_box : public widget<widget_flex_box, element_type::flex_box, widget_data, widget_flag::has_children | widget_flag::scissors>
        {
            using widget::widget;

            rect bounds_callback_impl(rect desiredBounds);

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
            label_data(utf8_string text) : text(std::move(text)) {}
        };

        struct widget_label : public widget<widget_label, element_type::label, label_data>
        {
            using widget::widget;

            void first_render_impl() const;
        };

        /// button

        struct button_data : widget_data
        {
            button_data() = default;
            // todo: const string& instead?
            button_data(utf8_string text) : text(std::move(text)) {}
            utf8_string text;
            friend struct widget_button;
        };

        struct widget_button : public widget<widget_button, element_type::button, button_data, widget_flag::scissors>
        {
        public:
            using widget::widget;

            void first_update_impl();
            void first_render_impl() const;
            void last_update_impl();

            callback<> on_click;
            callback<> on_double_click;
        private:
            bool _clicked = false;
            bool _double_clicked = false;
        };

        /// text box

        MX_MAKE_ENUM(text_box_action, uint_fast8_t,
            empty,
            write_whitespace,
            write_nonwhitespace,
            insert_block,
            erase_block,
            erase_left,
            erase_right,
            redo,
            undo
        );

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
            callback<const utf8_string&> on_change;
            const utf8_string& text() const;
        };

        // slider

        struct slider_data : widget_data
        {
        public:
            double minVal;
            double maxVal;
            double value;

            slider_data(double minVal, double maxVal, double value)
            : minVal(minVal)
            , maxVal(maxVal)
            , value(value)
            { }
        };

        struct widget_slider : public widget<widget_slider, element_type::slider, slider_data, widget_flag::has_children>
        {
        public:
            using widget::widget;

            rect bounds_callback_impl(rect desiredBounds);
            void last_update_impl();

            callback<double> on_change;
            double value() const;
        private:
            vec2 handle_center(double width) const;
            friend struct widget_slider_handle;

        };

        // range slider

        MX_MAKE_ENUM(range_slider_handle, uint_fast8_t,
            low,
            high
        );

        struct range_slider_data : widget_data
        {
        public:
            double minValue;
            double maxValue;

            double lowValue;
            double highValue;

            range_slider_data(double minValue, double maxValue, double lowValue, double highValue)
            : minValue(minValue)
            , maxValue(maxValue)
            , lowValue(lowValue)
            , highValue(highValue)
            { }
        private:
            // offset from the 'value point' of slider (right edge for low, left edge for high)
            double mouseDelta = 0.0;
            range_slider_handle_t focusedHandle = range_slider_handle::invalid;
            // size of the handles so we can correctly calculate the slider geometry
            double lowWidth = 0.0;
            double highWidth = 0.0;

            friend struct widget_range_slider;
            friend struct widget_range_slider_handle;
        };

        struct widget_range_slider : public widget<widget_range_slider, element_type::range_slider, range_slider_data, widget_flag::has_children>
        {
        public:
            using widget::widget;

            rect bounds_callback_impl(rect desiredBounds);
            void last_update_impl();

            callback<double,double> on_change;
            double lowValue() const;
            double highValue() const;
        private:
            friend struct widget_range_slider_handle;
            friend struct widget_slider_track;
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
            bool _clicked = false;

        };

        // context menu

        struct context_menu_data : widget_data
        {
            bool visible = true;
            vec2 spawnLocation;
        };

        struct widget_context_menu : public widget<widget_context_menu, element_type::context_menu, context_menu_data, widget_flag::has_children | widget_flag::windows>
        {
        public:
            using widget::widget;

            bool should_draw_impl() const;
            void handle_input_impl();
            void first_update_impl();
            void resolve_box_layout_impl(element_type_t et, widget_flag_t flag);
            rect bounds_callback_impl(rect desiredBounds);
            void first_render_impl() const;
            void last_render_impl() const;
        private:
            double internal_height = 0.0;
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
    if (imgui::X widget##__LINE__(PDATA); widget##__LINE__.draw(imgui::g_context.get_render_pass(),[&](auto& self){__VA_ARGS__;}))

#define MX_MULTIPASS_WIDGET_CODEGEN(X, PDATA, ...)\
    if (imgui::X widget##__LINE__(PDATA); true)\
        for (size_t i##__LINE__ = 0; i##__LINE__ <= static_cast<size_t>(imgui::render_pass::full); ++i##__LINE__)\
             if (widget##__LINE__.draw(static_cast<imgui::render_pass_t>(i##__LINE__), [&](auto& self) {__VA_ARGS__;}))

#define mxRoot(VIEWPORT) MX_WIDGET_CODEGEN(widget_root, VIEWPORT)
#define mxScrollArea(DATA, ...) MX_WIDGET_CODEGEN(widget_scroll_area, DATA, __VA_ARGS__)
#define mxHScroll(DATA, ...) MX_WIDGET_CODEGEN(widget_scroll_area, DATA, self.direction = imgui::scroll_direction::horizontal; __VA_ARGS__)
#define mxVScroll(DATA, ...) MX_WIDGET_CODEGEN(widget_scroll_area, DATA, self.direction = imgui::scroll_direction::vertical; __VA_ARGS__)
#define mxFlexBox(DATA, ...) MX_WIDGET_CODEGEN(widget_flex_box, DATA, __VA_ARGS__)
#define mxHBox(...) MX_WIDGET_CODEGEN(widget_flex_box, nullptr, self.direction = imgui::layout_direction::horizontal; __VA_ARGS__)
#define mxVBox(...) MX_WIDGET_CODEGEN(widget_flex_box, nullptr, self.direction = imgui::layout_direction::vertical; __VA_ARGS__)
#define mxSpacer(...) MX_WIDGET_CODEGEN(widget_spacer, nullptr, __VA_ARGS__)
#define mxLabel(DATA, ...) MX_WIDGET_CODEGEN(widget_label, DATA, __VA_ARGS__)
#define mxButton(DATA, ...) MX_WIDGET_CODEGEN(widget_button, DATA, __VA_ARGS__)
#define mxTextBox(DATA, ...) MX_WIDGET_CODEGEN(widget_text_box, DATA, __VA_ARGS__)
#define mxSlider(DATA, ...) MX_WIDGET_CODEGEN(widget_slider, DATA, __VA_ARGS__)
#define mxRangeSlider(DATA, ...) MX_WIDGET_CODEGEN(widget_range_slider, DATA, __VA_ARGS__)
#define mxCheckbox(DATA, ...) MX_WIDGET_CODEGEN(widget_checkbox, DATA, __VA_ARGS__)
#define mxContextMenu(DATA, ...) MX_MULTIPASS_WIDGET_CODEGEN(widget_context_menu, DATA, __VA_ARGS__)
