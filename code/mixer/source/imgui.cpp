#include "singleton.hpp"
#include "geometry.hpp"
#include "utf8_string.hpp"
#include "input.hpp"
#include "hash.hpp"
#include "nanovg.hpp"
#include "imgui.hpp"
#include "ui.hpp"

namespace tabletopify
{
    namespace imgui
    {
        // context

        void context::update(double dt)
        {
            MX_ASSERT(_widget_stack.size() == 0);
            MX_ASSERT(_render_scissor_stack.size() == 0);
        }

        void context::push_widget(widget_base* w)
        {
            _widget_stack.push_back(w);
        }

        void context::pop_widget()
        {
            _widget_stack.pop_back();
        }

        widget_base* context::parent_widget() const
        {
            MX_ASSERT(_widget_stack.size() > 0);
            return _widget_stack.back();
        }

        void context::focus_window_widget(widget_id_t id)
        {
            _focused_window_widget = id;
        }

        void context::unfocus_window_widget(widget_id_t id)
        {
            if (is_window_widget_focused(id)) {
                _focused_window_widget = widget_id::invalid;
            }
        }

        bool context::is_window_widget_focused(widget_id_t id) const
        {
            return id == _focused_window_widget;
        }

        void context::set_current_window_widget(widget_id_t id)
        {
            _current_window_widget = id;
        }

        bool context::is_current_window_widget_focused() const
        {
            return is_window_widget_focused(_current_window_widget);
        }

        void context::push_scissor(const rect& scissor)
        {
            _render_scissor_stack.push_back(scissor);
            g_nanovg.set_scissor(scissor);
        }

        void context::pop_scissor()
        {
            _render_scissor_stack.pop_back();
            g_nanovg.set_scissor(_render_scissor_stack.back());
        }

        // widget_base

        widget_base::widget_base(widget_data* data)
        : _data(data)
        {

        }

        rect widget_base::bounds_callback_impl(rect desired)
        {
            MX_FAIL_FAST("This method should never be called; widgets that support children must implement this themselves");
            return {};
        }

        void widget_base::focus_widget()
        {
            g_context.focus_widget(this->get_widget_id());
        }

         void widget_base::unfocus_widget()
        {
            g_context.unfocus_widget(this->get_widget_id());
        }

        bool widget_base::is_focused() const
        {
            return g_context.is_focused(this->get_widget_id());
        }

        void widget_base::resolve_style_impl(element_type_t et, element_state_t es)
        {
            this->_style = g_style_resolver.resolve(et, this->tags, es);
        }

        void widget_base::resolve_box_layout_impl(element_type_t et, widget_flag_t flag)
        {
            auto* parent = g_context.parent_widget();

            // get our box layout based off type and tags
            this->_layout = g_style_resolver.resolve_box_layout(et, this->tags);

            // override size if defined in style and not explicitly requested
            auto desiredBounds = this->_desiredBounds;
            if (!this->_desiredWidthRequested) desiredBounds.size.x = this->_layout.outerBounds.size.x;
            if (!this->_desiredHeightRequested) desiredBounds.size.y = this->_layout.outerBounds.size.y;

            // now get our actual bounds from parent
            this->_layout.outerBounds = parent->bounds_callback(desiredBounds);

            // increment the number of children our parent has
            parent->increment_child_count();

            // get our style
            this->_style = g_style_resolver.resolve(et, this->tags, element_state::none);
        }

        void widget_base::resolve_scissor_impl()
        {
            auto* parent = g_context.parent_widget();
            const auto& scissor = parent->childScissor();

            // calc the scissor geometry for widget specific content
            this->_scissor = get_intersecting_rect(scissor, this->_layout.outerBounds);
            this->_childScissor = get_intersecting_rect(scissor, this->_layout.inner_bounds());
        }

        element_state_t widget_base::resolve_element_state_impl()
        {
            const auto mouse_position = g_input.get_mouse_position(input_frame::current);

            if (point_inside_rect(
                    mouse_position,
                    this->scissor()) &&
                point_inside_rounded_rect(
                    mouse_position,
                    this->_layout.border_outer_bounds(),
                    this->_layout.borderRadius))
            {
                return element_state::hover;
            }
            return element_state::none;
        }

        void widget_base::render_box_layout_impl() const
        {
            const auto& style = this->style();

            if (style.borderSize > 0.0)
            {
                const auto strokeBounds = this->_layout.border_outer_bounds();
                auto fillBounds = strokeBounds;
                fillBounds.position += vec2{style.borderSize, style.borderSize} / 2.0;
                fillBounds.size -= vec2{style.borderSize, style.borderSize};

                if (style.backgroundImage == image_handle::invalid) {
                    g_nanovg.color_fill_rounded_rect(fillBounds, style.borderRadius, style.backgroundColor);
                } else {
                    g_nanovg.image_fill_rounded_rect(fillBounds, style.borderRadius, style.backgroundImage, style.backgroundImageMode);
                }
                g_nanovg.stroke_rounded_rect(strokeBounds, style.borderRadius, style.borderSize, style.borderColor);
            }
            else
            {
                auto fillBounds = this->_layout.border_outer_bounds();
                if (style.backgroundImage == image_handle::invalid) {
                    g_nanovg.color_fill_rounded_rect(fillBounds, style.borderRadius, style.backgroundColor);
                } else {
                    g_nanovg.image_fill_rounded_rect(fillBounds, style.borderRadius, style.backgroundImage, style.backgroundImageMode);
                }
            }
        }

        void widget_base::requestSize(const vec2& size)
        {
            _desiredBounds.size = size;
            _desiredWidthRequested = true;
            _desiredHeightRequested = true;
        }

        void widget_base::requestWidth(double width)
        {
            _desiredBounds.size.x = width;
            _desiredWidthRequested = true;
        }

        void widget_base::requestHeight(double height)
        {
            _desiredBounds.size.y = height;
            _desiredHeightRequested = true;
        }

        void widget_base::requestPosition(const vec2& position)
        {
            _desiredBounds.position = position;
        }

        void widget_base::requestBounds(const rect& bounds)
        {
            _desiredBounds = bounds;
            _desiredWidthRequested = true;
            _desiredHeightRequested = true;
        }

        // Root

        widget_root::widget_root(const rect& viewport)
        : widget_root(nullptr)
        {
            _viewport = viewport;
            _scissor = viewport;
            _childScissor = viewport;
            g_input.set_mouse_cursor(mouse_cursor::arrow);
        }

        widget_root::~widget_root()
        {
        }

        void widget_root::resolve_box_layout_impl(element_type_t et, widget_flag_t flag)
        {
            // get our box layout based off type and tags
            this->_layout = g_style_resolver.resolve_box_layout(et, this->tags);

            // now get our actual bounds from parent
            this->_layout.outerBounds = this->_viewport;

            // get our style
            this->_style = g_style_resolver.resolve(et, this->tags, element_state::none);
        }

        void widget_root::resolve_scissor_impl()
        {
            // calc the scissor geometry for widget specific content
            this->_scissor = this->_layout.outerBounds;
            this->_childScissor = this->_layout.inner_bounds();
        }

        rect widget_root::bounds_callback_impl(rect)
        {
            return this->_layout.child_bounds();
        }
    }
}