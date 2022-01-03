#include "../imgui.hpp"
#include "../input.hpp"

namespace dm
{
    namespace imgui
    {
        // Root

        widget_root::widget_root(const rect& viewport)
        : widget_root(nullptr)
        {
            _viewport = viewport;
            _scissor = viewport;
            _childScissor = viewport;
            g_input->set_mouse_cursor(mouse_cursor::arrow);
            g_context->focus_window_widget(this->get_widget_id());
        }

        void widget_root::resolve_box_layout_impl(render_pass_t, element_type_t et)
        {
            // get our style
            this->_style = g_style_resolver->resolve(et, this->tags, element_state::none);

            // get our box layout based off type and tags
            this->_layout = this->_style->get_box_layout();

            // now get our actual bounds from parent
            this->_layout.outerBounds = this->_viewport;
        }

        draw_layer_t widget_root::get_draw_layer_impl() const
        {
            return draw_layer::background;
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