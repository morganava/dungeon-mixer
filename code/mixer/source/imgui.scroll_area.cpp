#include "imgui.hpp"
#include "input.hpp"

namespace tabletopify
{
    namespace imgui
    {
        // scroll_area

        void widget_scroll_area::resolve_scissor_impl()
        {
            const auto& rb = g_context.peek_render_bounds();
            const auto& scissor = rb.widget->scissor();

            // need to exclude the scroll area's scroll bars from scissor
            const bool scroll_vertical = static_cast<bool>(this->data()->direction & scroll_direction::vertical);
            const bool scroll_horizontal = static_cast<bool>(this->data()->direction & scroll_direction::horizontal);
            const auto scrollbar_size = this->style().scrollbarSize;

            // calc the scissor geometry for widget specific contetn
            // but only scissor the rendering if we're a certain type of element
            const auto padding = this->_layout.padding;
            this->_layout.padding = edges{0};
            auto childBounds = this->_layout.inner_bounds();
            this->_layout.padding = padding;
            childBounds.position += vec2{padding.left,padding.top};
            childBounds.size -= (vec2{padding.left,padding.top} + vec2{padding.right,padding.bottom});

            if (scroll_vertical)
            {
                childBounds.size.x -= scrollbar_size;
            }
            if (scroll_horizontal)
            {
                childBounds.size.y -= scrollbar_size;
            }

            this->_scissor = get_intersecting_rect(scissor, this->_layout.outerBounds);
            this->_childScissor = get_intersecting_rect(scissor, childBounds);
        }

        void widget_scroll_area::first_update_impl()
        {
            for(auto evt : g_input.get_input_events())
            {
                if (evt.type() == input_event_type::mouse_single_click)
                {
                    if (point_inside_rect(evt.position(), this->childScissor()))
                    {
                        this->focus_widget();
                    }
                    else
                    {
                        this->unfocus_widget();
                    }
                }
            }
        }

        rect widget_scroll_area::bounds_callback(widget_base* parent, const rect& desired)
        {
            auto& self = *static_cast<widget_scroll_area*>(parent);
            const auto& data = *self.data();
            const auto inner_bounds = self._layout.inner_bounds();
            const auto scrollbar_size = self.style().scrollbarSize;

            rect retval;
            switch(data.direction)
            {
                // stack vertically
                case scroll_direction::vertical:
                {
                    retval = inner_bounds;
                    retval.position.y += self.vertical.internal_size - data.vertical.scroll_offset;
                    retval.size.x -= scrollbar_size;
                    retval.size.y = desired.size.y;

                    self.vertical.internal_size += desired.size.y;
                    break;
                }
                // stack horizontally
                case scroll_direction::horizontal:
                {
                    retval = inner_bounds;
                    retval.position.x += self.horizontal.internal_size - data.horizontal.scroll_offset;
                    retval.size.x = desired.size.x;
                    retval.size.y -= scrollbar_size;

                    self.horizontal.internal_size += desired.size.x;
                    break;
                }
                // place wherever desired
                case scroll_direction::both:
                {
                    retval = desired;

                    self.vertical.internal_size = std::max(self.vertical.internal_size, retval.bottom());
                    self.horizontal.internal_size = std::max(self.horizontal.internal_size, retval.right());

                    retval.position += inner_bounds.position - vec2{data.horizontal.scroll_offset, data.vertical.scroll_offset};
                    break;
                }
                default:
                {
                    MX_FAIL_FAST("Unhandled direction : {}", static_cast<uint_fast8_t>(data.direction));
                }
            }

            return retval;
        }

        void widget_scroll_area::last_update_impl()
        {
            // style constants
            const auto scrollbar_size = this->style().scrollbarSize;
            const auto scrollbar_min_length = this->style().scrollbarMinLength;
            const auto scroll_step = this->style().scrollbarStep;

            const auto bounds = this->_layout.border_inner_bounds();
            const bool scroll_vertical = static_cast<bool>(this->data()->direction & scroll_direction::vertical);
            const bool scroll_horizontal = static_cast<bool>(this->data()->direction & scroll_direction::horizontal);

            // valid scrollable range  (0 to scrollable_height)

            auto calc_scollabale_size = [](double internal_size, double bounds_size, double scrollbar_size) -> double
            {
                double val = internal_size - bounds_size + scrollbar_size;
                if (val < 0.0) val = 0.0;
                return val;
            };

            const auto scrollable_height = calc_scollabale_size(this->vertical.internal_size, bounds.height(), scroll_vertical ? 0.0  : scrollbar_size);
            const auto scrollable_width = calc_scollabale_size(this->horizontal.internal_size, bounds.width(), scroll_horizontal ? 0.0  : scrollbar_size);

            // size: length of the scrollbar
            // view_size: length of the viewport
            auto calc_scrollbar_grab_length = [&](double size, double view_size) -> double
            {
                if (view_size == 0.0) {
                    return size;
                }
                return clamp(scrollbar_min_length, size*size / view_size, size);
            };

            // shorthand names
            auto& vsa = this->vertical.scrollbar_area;
            auto& vsga = this->vertical.scrollbar_grab_area;

            if (scroll_vertical)
            {
                vsa =
                {
                    {bounds.right() - scrollbar_size, bounds.top()},
                    {scrollbar_size, bounds.height() - (scroll_horizontal ? scrollbar_size : 0.0)}
                };
                const double grab_length = calc_scrollbar_grab_length(vsa.height(), this->vertical.internal_size);
                vsga = (scrollable_height == 0.0) ? vsa :
                rect{
                    {vsa.left(), vsa.top() + (vsa.height() - grab_length) * (data()->vertical.scroll_offset / scrollable_height)},
                    {vsa.width(), grab_length}
                };
            }

            // shorthand names

            auto& hsa = this->horizontal.scrollbar_area;
            auto& hsga = this->horizontal.scrollbar_grab_area;

            if (scroll_horizontal)
            {
                hsa =
                {
                    {bounds.left(), bounds.bottom() - scrollbar_size},
                    {bounds.width() - (scroll_vertical ? scrollbar_size : 0.0), scrollbar_size}
                };
                const double grab_length = calc_scrollbar_grab_length(hsa.width(), this->horizontal.internal_size);
                hsga =
                {
                    {hsa.left() + (hsa.width() - grab_length) * (data()->horizontal.scroll_offset / scrollable_width), hsa.top()},
                    {grab_length, hsa.height()}
                };
            }

            /* handle input to adjust the scroll offset */

            auto prev_mouse_down = g_input.get_mouse_button_state(mouse_button::left, input_frame::previous);
            auto current_mouse_down = g_input.get_mouse_button_state(mouse_button::left, input_frame::current);
            auto current_mouse_position = g_input.get_mouse_position(input_frame::current);

            if (scroll_vertical)
            {
                if (this->vertical.internal_size < vsa.height())
                {
                    data()->vertical.scroll_offset = 0.0;
                }
                else
                {
                    // handle click and drag
                    if (!data()->vertical.grabbing)
                    {
                        // mouse clicked
                        if (!prev_mouse_down && current_mouse_down)
                        {
                            // mouse in scrollbar grab area
                            if (point_inside_rect(current_mouse_position, vsga))
                            {
                                data()->vertical.grabbing = true;
                                data()->vertical.grab_offset = current_mouse_position.y - vsga.top();
                            }
                            // mouse click on scrollbar area
                            else if(point_inside_rect(current_mouse_position, vsa))
                            {
                                data()->vertical.grabbing = true;
                                data()->vertical.grab_offset = vsga.height() / 2.0;
                                data()->vertical.scroll_offset = (current_mouse_position.y - vsa.top() - data()->vertical.grab_offset) / (vsa.height() - vsga.height()) * scrollable_height;
                            }
                        }
                    }
                    else
                    {
                        // release scrollbar
                        if(!current_mouse_down)
                        {
                            data()->vertical.grabbing = false;
                        }
                        // drag scroll
                        else
                        {
                            data()->vertical.scroll_offset = (current_mouse_position.y - vsa.top() - data()->vertical.grab_offset) / (vsa.height() - vsga.height()) * scrollable_height;
                        }
                    }
                }
            }
            if (scroll_horizontal)
            {
                if (this->horizontal.internal_size < hsa.width())
                {
                    data()->horizontal.scroll_offset = 0.0;
                }
                else
                {
                    // handle click and drag
                    if (!data()->horizontal.grabbing)
                    {
                        // mouse clicked
                        if(!prev_mouse_down && current_mouse_down)
                        {
                            // mouse in scrollbar grab area
                            if (point_inside_rect(current_mouse_position, hsga))
                            {
                                data()->horizontal.grabbing = true;
                                data()->horizontal.grab_offset = current_mouse_position.x - hsga.left();
                            }
                            // mouse click on scrollbar area
                            else if (point_inside_rect(current_mouse_position, hsa))
                            {
                                data()->horizontal.grabbing = true;
                                data()->horizontal.grab_offset = hsga.width() / 2.0;
                                data()->horizontal.scroll_offset = (current_mouse_position.x - hsa.left() - data()->horizontal.grab_offset) / (hsa.width() - hsga.width()) * scrollable_width;
                            }
                        }
                    }
                    else
                    {
                        // release scrollbar
                        if(!current_mouse_down)
                        {
                            data()->horizontal.grabbing = false;
                        }
                        // drag scroll
                        else
                        {
                            data()->horizontal.scroll_offset = (current_mouse_position.x - hsa.left() - data()->horizontal.grab_offset) / (hsa.width() - hsga.width()) * scrollable_width;
                        }
                    }
                }
            }

            if (scroll_horizontal || scroll_vertical)
            {
                // handle keyboard input
                if (this->is_focused())
                {
                    for(auto evt : g_input.get_input_events())
                    {
                        if (evt.type() == input_event_type::key_down ||
                            evt.type() == input_event_type::key_repeat)
                        {
                            const auto keycode = evt.keycode();
                            if (scroll_vertical)
                            {
                                switch(keycode)
                                {
                                    case GLFW_KEY_PAGE_UP:
                                        data()->vertical.scroll_offset -= 5 * scroll_step;
                                        break;
                                    case GLFW_KEY_PAGE_DOWN:
                                        data()->vertical.scroll_offset += 5 * scroll_step;
                                        break;
                                    case GLFW_KEY_UP:
                                        data()->vertical.scroll_offset -= scroll_step;
                                        break;
                                    case GLFW_KEY_DOWN:
                                        data()->vertical.scroll_offset += scroll_step;
                                        break;
                                    case GLFW_KEY_HOME:
                                        data()->vertical.scroll_offset = 0;
                                        break;
                                    case GLFW_KEY_END:
                                        data()->vertical.scroll_offset = scrollable_height;
                                        break;
                                }
                            }
                            if (scroll_horizontal)
                            {
                                switch(keycode)
                                {
                                    case GLFW_KEY_LEFT:
                                        data()->horizontal.scroll_offset -= scroll_step;
                                        break;
                                    case GLFW_KEY_RIGHT:
                                        data()->horizontal.scroll_offset += scroll_step;
                                        break;
                                }
                            }
                        }
                    }
                }

                // handle scroll wheel
                if (point_inside_rect(current_mouse_position, get_intersecting_rect(this->scissor(), this->_layout.border_inner_bounds())))
                {
                    if (!data()->vertical.grabbing)
                    {
                        data()->vertical.scroll_offset -= g_input.get_mouse_scroll(input_frame::current).y * scroll_step;
                    }
                    if (!data()->horizontal.grabbing)
                    {
                        data()->horizontal.scroll_offset -= g_input.get_mouse_scroll(input_frame::current).x * scroll_step;
                    }
                }
            }

             // clamp offsets
            data()->vertical.scroll_offset = clamp<double>(0.0, data()->vertical.scroll_offset, scrollable_height);
            data()->horizontal.scroll_offset = clamp<double>(0.0, data()->horizontal.scroll_offset, scrollable_width);
        }

        void widget_scroll_area::last_render_impl() const
        {
            const bool scroll_vertical = static_cast<bool>(this->data()->direction & scroll_direction::vertical);
            const bool scroll_horizontal = static_cast<bool>(this->data()->direction & scroll_direction::horizontal);
            const auto back_color = this->style().scrollbarBackColor;
            const auto fore_color = this->style().scrollbarForeColor;
            const auto active_color = this->style().scrollbarActiveColor;
            const auto corner_radius = this->style().scrollbarCornerRadius;

            const auto vsa = this->vertical.scrollbar_area;
            const auto vsga = this->vertical.scrollbar_grab_area;
            const auto hsa = this->horizontal.scrollbar_area;
            const auto hsga = this->horizontal.scrollbar_grab_area;

            if (scroll_vertical)
            {
                g_nanovg.color_fill_rect(vsa, back_color);

                const auto grab_color = data()->vertical.grabbing ? active_color : fore_color;
                g_nanovg.color_fill_rounded_rect(vsga, corner_radius, grab_color);
            }
            if (scroll_horizontal)
            {
                g_nanovg.color_fill_rect(hsa, back_color);

                const auto grab_color = data()->horizontal.grabbing ? active_color : fore_color;
                g_nanovg.color_fill_rounded_rect(hsga, corner_radius, grab_color);
            }

            if(scroll_vertical && scroll_horizontal)
            {
                const auto scrollbar_size = this->style().scrollbarSize;
                auto bottom_corner = rect{vsa.bottom_left(),vec2{scrollbar_size}};
                g_nanovg.color_fill_rect(bottom_corner, back_color);
            }
        }
    }
}