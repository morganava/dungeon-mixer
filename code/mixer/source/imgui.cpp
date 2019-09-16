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
        widget_id_t next_widget_id()
        {
            static widget_id_t nextId = 0;
            return nextId++;
        }

        // render_bounds

        rect render_bounds::eval(const rect& desired) const
        {
            return this->callback(this->widget, desired);
        }

        // context

        void context::update(double dt)
        {
            MX_ASSERT(_render_bounds_stack.size() == 0);
        }

        void context::push_render_bounds(const render_bounds& rb)
        {
            _render_bounds_stack.push_back(rb);
        }

        const render_bounds& context::peek_render_bounds() const
        {
            return _render_bounds_stack.back();
        }

        void context::pop_render_bounds()
        {
            _render_bounds_stack.pop_back();
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
            const auto& rb = g_context.peek_render_bounds();

            // get our box layout based off type and tags
            this->_layout = g_style_resolver.resolve_box_layout(et, this->tags);

            // override size if defined in style and not explicitly requested
            auto desiredBounds = this->_desiredBounds;
            if (!this->_desiredWidthRequested) desiredBounds.size.x = this->_layout.outerBounds.size.x;
            if (!this->_desiredHeightRequested) desiredBounds.size.y = this->_layout.outerBounds.size.y;

            // now get our actual bounds from parent
            this->_layout.outerBounds = rb.eval(desiredBounds);

            // get our style
            this->_style = g_style_resolver.resolve(et, this->tags, element_state::none);
        }

        void widget_base::resolve_scissor_impl()
        {
            const auto& rb = g_context.peek_render_bounds();
            const auto& scissor = rb.widget->childScissor();
            // calc the scissor geometry for widget specific content
            this->_scissor = get_intersecting_rect(scissor, this->_layout.outerBounds);
            this->_childScissor = get_intersecting_rect(scissor, this->_layout.inner_bounds());
        }

        void widget_base::render_box_layout_impl() const
        {
            const auto& style = this->style();

            if (style.borderSize > 0.0)
            {
                auto strokeBounds = this->_layout.border_outer_bounds();
                auto fillBounds = strokeBounds;
                fillBounds.position += vec2{style.borderSize, style.borderSize} / 2.0;
                fillBounds.size -= vec2{style.borderSize, style.borderSize};

                g_nanovg.fill_rounded_rect(fillBounds, style.borderRadius, style.backgroundColor);
                g_nanovg.stroke_rounded_rect(strokeBounds, style.borderRadius, style.borderSize, style.borderColor);
            }
            else
            {
                auto fillBounds = this->_layout.border_outer_bounds();
                g_nanovg.fill_rounded_rect(fillBounds, style.borderRadius, style.backgroundColor);
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
            g_context.push_render_bounds({&widget_root::root_bounds_callback, this});
        }

        widget_root::~widget_root()
        {
            g_context.pop_render_bounds();
        }

        rect widget_root::bounds_callback(widget_base* parent, const rect& desired)
        {
            auto& self = *static_cast<widget_root*>(parent);
            return self._layout.child_bounds();
        }

        rect widget_root::root_bounds_callback(widget_base* parent, const rect& desired)
        {
            auto& self = *static_cast<widget_root*>(parent);
            return self._viewport;
        }
    }
}