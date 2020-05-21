#include "imgui.hpp"
#include "input.hpp"

namespace tabletopify
{
    namespace imgui
    {
        bool widget_context_menu::should_draw_impl() const
        {
            return this->data()->visible;
        }

        void widget_context_menu::first_update_impl()
        {
            for(auto evt : g_input.get_input_events())
            {
                switch(evt.type())
                {
                    case input_event_type::mouse_down:
                    {
                        if (!point_inside_rect(evt.position(), this->_layout.outerBounds))
                        {
                            worker(worker_id::ui) << [data=this->data()]()
                            {
                                data->visible = false;
                            };
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
        }

        void widget_context_menu::resolve_box_layout_impl(element_type_t et, widget_flag_t flags)
        {
            auto* parent = g_context.parent_widget();

            // get our box layout based off type and tags
            this->_layout = g_style_resolver.resolve_box_layout(et, this->tags);

            // override size if defined in style and not explicitly requested
            auto desiredBounds = this->_desiredBounds;
            if (!this->_desiredWidthRequested) desiredBounds.size.x = this->_layout.outerBounds.size.x;

            // now get our actual bounds from parent
            this->_layout.outerBounds.position = this->data()->spawnLocation;
            this->_layout.outerBounds.size = desiredBounds.size;

            // calculate the real bounds
            if (g_context.get_render_pass() == render_pass::geometry)
            {
                this->data()->spawnLocation = {128,128};
            }
            else if (g_context.get_render_pass() == render_pass::full)
            {
                // we need to pop our old bounds on the second pass
                g_context.pop_widget();

                // stored height is what we need our inner height to be, we need to add border,margin and padding
                // to get our final outer height
                const auto& layout = this->_layout;
                const double diff = layout.margin.top + layout.margin.bottom + 2* layout.borderSize + layout.padding.top + layout.padding.bottom;

                this->_layout.outerBounds.size.y = this->internal_height + diff;

				// reset internal height to 0 for this render pass
                this->internal_height = 0.0;
            }

            // increment the number of children our parent has
            parent->increment_child_count();

            // get our style
            this->_style = g_style_resolver.resolve(et, this->tags, element_state::none);
        }

        rect widget_context_menu::bounds_callback_impl(rect desired)
        {
            auto childBounds = _layout.child_bounds();

            rect retval =
            {
                {childBounds.left(), childBounds.top() + internal_height},
                {childBounds.width(), desired.height()}
            };

            internal_height += desired.height();

            return retval;
        }

        void widget_context_menu::first_render_impl() const
        {
            MX_ASSERT(g_context.get_render_pass() == render_pass::full);
            g_nanovg.set_draw_layer(draw_layer::foreground);
        }

        void widget_context_menu::last_render_impl() const
        {
            MX_ASSERT(g_context.get_render_pass() == render_pass::full);
            g_nanovg.set_draw_layer(draw_layer::middleground);
        }
    }
}