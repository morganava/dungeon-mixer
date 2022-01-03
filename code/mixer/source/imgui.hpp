#pragma once

#include "stylesheet.hpp"
#include "imgui/callback.hpp"

namespace dm
{
    // immmediate mode gui
    namespace imgui
    {
        SMOL_MAKE_ENUM(render_pass, uint_fast8_t,
            single, /* a full pass for widgets that only require a single pass */
            multi_geometry, /* just do the geometry portion */
            multi_full /* a full pass for widgets requiring two passes */
        );

        // widget id
        SMOL_MAKE_HANDLE_ENUM(widget_id,
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

            void push_draw_layer(draw_layer_t layer);
            void pop_draw_layer();

            void push_render_pass(render_pass_t rp);
            void pop_render_pass();
            render_pass_t get_render_pass() const;

        private:
            std::vector<widget_base*> _widget_stack;
            std::vector<rect> _render_scissor_stack;
            std::vector<draw_layer_t> _draw_layer_stack;
            std::vector<render_pass_t> _render_pass_stack;

            widget_id_t _focused_window_widget = widget_id::invalid;
            widget_id_t _focused_widget = widget_id::invalid;
            widget_id_t _current_window_widget = widget_id::invalid;
        };
    }
    MX_MAKE_SERVICE_IMPL(imgui::context, context, worker_flag::ui);
    namespace imgui
    {
        struct widget_data
        {
            widget_id_t id = next_widget_id();
        };

        SMOL_MAKE_FLAG_ENUM(widget_flag, uint32_t,
            has_children = SMOL_FLAG(0),    // may have child widgets
            scissors     = SMOL_FLAG(1),    // defines a scissor region
            windows      = SMOL_FLAG(2),    // defines a window(?)
            draw_layers  = SMOL_FLAG(3),    // modifies the draw layer for self and children
            multipass    = SMOL_FLAG(4),    // separate geometry and render passes
        );

        // widget base-class

        struct widget_base
        {

        protected:
            widget_base() = default;
            constexpr const style_sheet& style() const {return *_style;}

            // virtual funcs
            virtual rect bounds_callback(rect desiredBounds) = 0;

            // widget interface methods that implementing widgets can override
            void resolve_box_layout_impl(render_pass_t renderPass, element_type_t et);
            void resolve_scissor_impl();
            element_state_t resolve_element_state_impl();
            void render_box_layout_impl() const;
            rect bounds_callback_impl(rect desired);
            void handle_input_impl();
            void resolve_style_impl(element_type_t, element_state_t);
            draw_layer_t get_draw_layer_impl() const;
            void render_impl() const;
            void draw_inner_widgets_impl();
            void fire_callbacks_impl() const;
        public:
            void requestSize(const vec2& size);
            void requestWidth(double width);
            void requestHeight(double height);
            void requestPosition(const vec2& position);
            void requestBounds(const rect& bounds);

            constexpr rect scissor() const {return _scissor; }
            constexpr rect childScissor() const {return _childScissor; }

            array<smol::utf8_string> tags;
            bool disabled = false;

            void increment_child_count() { _childCounter++; }

            constexpr widget_id_t get_widget_id() const
            {
                return _data ? _data->id : widget_id::null;
            }
            void focus_widget()
            {
                g_context->focus_widget(this->get_widget_id());
            }

            void unfocus_widget()
            {
                g_context->unfocus_widget(this->get_widget_id());
            }

            bool is_focused() const
            {
                return g_context->is_focused(this->get_widget_id());
            }

        protected:
            const style_sheet* _style = nullptr;
            box_layout _layout = {};
            rect _scissor = {};
            rect _childScissor = {};
            rect _desiredBounds = {};
            bool _desiredWidthRequested = false;
            bool _desiredHeightRequested = false;
            size_t _childCounter = 0;
            widget_data* const _data = nullptr;
            widget_base(widget_data* const data)
            : _data(data) { }
        };

        template<typename T, element_type ELEMENT_TYPE, typename WIDGET_DATA=widget_data, widget_flag_t WIDGET_FLAGS=widget_flag::none>
        struct widget : public widget_base
        {
        public:
            typedef WIDGET_DATA widget_data_t;

            widget(const widget&) = delete;
            widget& operator=(const widget&) = delete;

            constexpr widget(widget_data_t* pdata)
            : widget_base(pdata)
            , _widget_data(std::nullopt)
            { }

            constexpr widget(widget_data_t&& rdata)
            : widget_base(_widget_data.operator->())
            , _widget_data(std::make_optional(std::move(rdata)))
            { }

            // destructor called after child widgets have been handled
            ~widget()
            {
                auto& self = static_cast<T&>(*this);

                // draw any inner widgets we may have
                self.draw_inner_widgets();
                // call any user-provided callbacks
                self.fire_callbacks();

                self.cleanup_previous_render_pass();
            }

            // gets the list of render passes to perform given the current
            // render pass
            smol::enum_values<render_pass_t> get_passes(render_pass_t currentRenderPass)
            {
                // multipass widgets
                if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::multipass))
                {
                    // begin multipass for this widget and children
                    if (currentRenderPass == render_pass::single)
                    {
                        return smol::enum_range(render_pass::multi_geometry, 2);
                    }
                }
                return smol::enum_range(currentRenderPass, 1);
            }

            // internalish draw methods
            bool draw(render_pass_t renderPass, auto&& lambda)
            {
                auto& self = static_cast<T&>(*this);

                // perform work required at the start of a render pass
                self.begin_render_pass(renderPass);

                // do any user work here
                lambda(self);

                // figure out this widget's box layout (geometry)
                self.resolve_box_layout(renderPass);

                switch(renderPass)
                {
                    case render_pass::multi_geometry:
                        break;
                    case render_pass::multi_full:
                        [[fallthrough]];
                    case render_pass::single:
                        // calculate our our scissor regions
                        self.resolve_scissor();
                        // handle input events
                        self.handle_input();
                        // resovle our style based off of widgets tags and input state
                        self.resolve_style();
                        // maybe update the draw layer this (and child) widgets draw to
                        self.update_draw_layer();
                        // common box layout rendering
                        self.render_box_layout();
                        // widget-specific rendering
                        self.render();
                        break;
                    default:
                        SMOL_FAIL_FAST("Unexpected render pass: {}", renderPass);
                        break;

                }
                self.end_render_pass(renderPass);

                return true;
            }
        private:
            // when performing multiple passes (eg once for geometry, once for rendering)
            // we may need to cleanup our leftover state
            void cleanup_previous_render_pass()
            {
                // previus pass-depedendent cleanup
                switch(previous_render_pass_)
                {
                    case render_pass::invalid: return;;
                    case render_pass::multi_geometry:
                    {
                        // no geometry-pass specific cleanup to do
                        break;
                    }
                    case render_pass::single: [[fallthrough]];
                    case render_pass::multi_full:
                    {
                        if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::scissors))
                        {
                            g_context->pop_scissor();
                        }
                        if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::draw_layers))
                        {
                            g_context->pop_draw_layer();
                        }
                        break;
                    }
                    default:
                    {
                        SMOL_FAIL_FAST("Invalid previous render pass: {}", previous_render_pass_);
                        break;
                    }
                }

                // common cleanup work

                if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::has_children))
                {
                    // we need to pop our old bounds on the second pass
                    g_context->pop_widget();

                    // need to reset our child counter
                    _childCounter = 0;
                }
                if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::windows))
                {
                    g_context->set_current_window_widget(widget_id::invalid);
                }
                g_context->pop_render_pass();
            }

            void begin_render_pass(render_pass_t renderPass)
            {
                // cleanup any leftover state this widget left behind on the previous render pass
                this->cleanup_previous_render_pass();

                g_context->push_render_pass(renderPass);

                if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::windows))
                {
                    g_context->set_current_window_widget(this->get_widget_id());
                }
            }

            void end_render_pass(render_pass_t renderPass)
            {
                if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::scissors))
                {
                    if (renderPass == render_pass::single ||
                        renderPass == render_pass::multi_full)
                    {
                        g_context->push_scissor(this->childScissor());
                    }
                }
                if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::has_children))
                {
                    g_context->push_widget(this);

                }
                previous_render_pass_ = renderPass;
            }

            // callback for child widgets to get their draw area
            rect bounds_callback(rect desiredBounds) override
            {
                auto self = static_cast<T*>(this);
                return self->bounds_callback_impl(desiredBounds);
            }

            // Wrappers around widget *_impl methods

            // calculates the physical dimensions of the widget and the scissor areas
            void resolve_box_layout(render_pass_t renderPass)
            {
                static_cast<T*>(this)->resolve_box_layout_impl(renderPass, ELEMENT_TYPE);
            }

            // calculate scissor regions for widget
            void resolve_scissor()
            {
                static_cast<T*>(this)->resolve_scissor_impl();
            }

            void handle_input()
            {
                if (!this->disabled)
                {
                    static_cast<T*>(this)->handle_input_impl();
                }
            }

            // resolve this widget's style
            void resolve_style()
            {
                auto self = static_cast<T*>(this);
                self->resolve_style_impl(ELEMENT_TYPE, self->resolve_element_state_impl());
            }

            void update_draw_layer()
            {
                if constexpr (static_cast<bool>(WIDGET_FLAGS & widget_flag::draw_layers))
                {
                    g_context->push_draw_layer(static_cast<T*>(this)->get_draw_layer_impl());
                }
            }

            // render's the box layout determined bdy the style
            void render_box_layout() const
            {
                static_cast<const T*>(this)->render_box_layout_impl();
            }

            // widget specific rendering
            void render() const
            {
                static_cast<const T*>(this)->render_impl();
            }

            void draw_inner_widgets()
            {
                static_cast<T*>(this)->draw_inner_widgets_impl();
            }

            void fire_callbacks() const
            {
                static_cast<const T*>(this)->fire_callbacks_impl();
            }

        protected:
            // widget methods
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
            render_pass_t previous_render_pass_ = render_pass::invalid;
        };

        // the root widget with viewport info
        struct widget_root : public widget<widget_root, element_type::root, widget_data, widget_flag::has_children | widget_flag::scissors | widget_flag::windows | widget_flag::draw_layers>
        {
            using widget::widget;
            widget_root(const rect& viewport);

            draw_layer_t get_draw_layer_impl() const;
            void resolve_box_layout_impl(render_pass_t renderPass, element_type_t et);
            void resolve_scissor_impl();

            rect bounds_callback_impl(rect desiredBounds);
        private:
            rect _viewport;
        };

        //
        // Layout Widgets
        //

        // scroll_area
        SMOL_MAKE_FLAG_ENUM(scroll_direction, uint_fast8_t,
            horizontal = SMOL_FLAG(0),
            vertical   = SMOL_FLAG(1),
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
            void handle_input_impl();
            rect bounds_callback_impl(rect desiredBounds);
            void draw_inner_widgets_impl();

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
            smol::utf8_string text;
            label_data() = default;
            label_data(smol::utf8_string text) : text(std::move(text)) {}
        };

        struct widget_label : public widget<widget_label, element_type::label, label_data>
        {
            using widget::widget;

            void render_impl() const;
        };

        /// button

        struct button_data : widget_data
        {
            button_data() = default;
            // todo: const string& instead?
            button_data(smol::utf8_string text) : text(std::move(text)) {}
            smol::utf8_string text;
            friend struct widget_button;
        };

        struct widget_button : public widget<widget_button, element_type::button, button_data, widget_flag::scissors>
        {
        public:
            using widget::widget;

            void handle_input_impl();
            void render_impl() const;
            void fire_callbacks_impl() const;

            callback<> on_click;
            callback<> on_double_click;
        private:
            bool _clicked = false;
            bool _double_clicked = false;
        };

        /// text box

        SMOL_MAKE_ENUM(text_box_action, uint_fast8_t,
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
            smol::utf8_stringbuffer sb;
            smol::utf8_string text;
            smol::utf8_string placeholderText;
            double textOffset = 0.0;

            size_t selectBegin = 0;
            size_t selectEnd = 0;
            text_box_action_t previousAction = text_box_action::empty;
            struct snapshot
            {
                smol::utf8_string text;
                size_t selectBegin;
                size_t selectEnd;
            };
            std::vector<snapshot> undoStack;
            std::vector<snapshot> redoStack;
            text_box_data() = default;
            text_box_data(smol::utf8_string text, smol::utf8_string placeholderText)
            : sb(text)
            , text(std::move(text))
            , placeholderText(std::move(placeholderText))
            {}
        };

        struct widget_text_box : public widget<widget_text_box, element_type::text_box, text_box_data>
        {
        public:
            using widget::widget;

            void handle_input_impl();
            void render_impl() const;

            callback<const smol::utf8_string&> on_submit;
            callback<const smol::utf8_string&> on_change;
            const smol::utf8_string& text() const;
        };

        // slider

        struct slider_data : widget_data
        {
        public:
            double minVal = 0.0;
            double maxVal = 1.0;
            double value = 1.0;

            slider_data() = default;
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
            void draw_inner_widgets_impl();

            callback<double> on_change;
            double value() const;
        private:
            vec2 handle_center(double width) const;
            friend struct widget_slider_handle;

        };

        // range slider

        SMOL_MAKE_ENUM(range_slider_handle, uint_fast8_t,
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
            void draw_inner_widgets_impl();

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
            bool checked = false;
            checkbox_data() = default;
            checkbox_data(bool checked)
            : checked(checked)
            { }
        };

        struct widget_checkbox : public widget<widget_checkbox, element_type::checkbox, checkbox_data>
        {
        public:
            using widget::widget;

            element_state_t resolve_element_state_impl();
            void handle_input_impl();
            void fire_callbacks_impl() const;

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

        struct widget_context_menu : public widget<widget_context_menu, element_type::context_menu, context_menu_data, widget_flag::has_children | widget_flag::windows | widget_flag::draw_layers | widget_flag::multipass>
        {
        public:
            using widget::widget;

            bool should_draw_impl() const;
            void handle_input_impl();
            void resolve_box_layout_impl(render_pass_t renderPass, element_type_t et);
            rect bounds_callback_impl(rect desiredBounds);
            draw_layer_t get_draw_layer_impl() const;
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
    if (imgui::X widget##__LINE__(PDATA); widget##__LINE__.draw(g_context->get_render_pass(), [&](auto& self){__VA_ARGS__;}))

#define MX_MULTIPASS_WIDGET_CODEGEN(X, PDATA, ...)\
    if (imgui::X __widget(PDATA); true)\
        for (auto pass : __widget.get_passes(g_context->get_render_pass()))\
            if (__widget.draw(pass, [&](auto& self) {__VA_ARGS__;}))

// #define mxRoot(VIEWPORT) MX_WIDGET_CODEGEN(widget_root, VIEWPORT)
#define mxRoot(VIEWPORT) if (imgui::widget_root __root(VIEWPORT); __root.draw(imgui::render_pass::single, [](auto& self){}))

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
#define mxDropdownBox(DATA, ...) MX_MULTIPASS_WIDGET_CODEGEN(widget_dropdown_box, DATA, __VA_ARGS__)

