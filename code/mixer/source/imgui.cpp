#include "singleton.hpp"
#include "geometry.hpp"
#include "utilities.hpp"
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
            return this->callback(this->callback_data, desired);
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

        void context::set_render_scissor(const rect& region)
        {
            get_nanovg()->set_scissor(region);
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
            g_context.focus_widget(this->get_widget_id());
        }

        bool widget_base::is_focused() const
        {
            return g_context.is_focused(this->get_widget_id());
        }

        void widget_base::destructor_impl(void(*func)(void*))
        {
            // restore the scissor area for second render pass
            scissor = g_context.peek_render_bounds().scissor;
            g_context.set_render_scissor(scissor);

            // post-child updates and renders
            func(this);

            g_context.pop_render_bounds();   
        } 

        void widget_base::calc_render_bounds_impl(render_bounds(*func)(void*))
        { 
            const auto& rb = g_context.peek_render_bounds();

            this->bounds = rb.eval(this->bounds);
            const auto& parent_scissor = rb.scissor;

            // render the border and bgcolor
            auto marginScissor = get_intersecting_rect(parent_scissor, this->bounds);
            get_nanovg()->set_scissor(marginScissor);
            this->bounds = this->render_border(bounds);

            // calc the scissor geometry for widget specific contetn
            this->scissor = get_intersecting_rect(parent_scissor, this->bounds);
            get_nanovg()->set_scissor(this->scissor);

            // push info for child render bounds calculations
            g_context.push_render_bounds(func(this));
        }

        void widget_base::resolve_style()
        {
            this->_style = g_style_resolver.resolve();
        }

        rect widget_base::render_border(const rect& desiredBounds) const
        {
            const auto borderSize = this->_style->borderSize;
            const auto borderRadius = this->_style->borderRadius;
            const auto backgroundColor = this->_style->backgroundColor;

            if (borderSize > 0.0)
            {
                const auto borderColor = this->_style->borderColor;

                rect borderBounds = desiredBounds;
                borderBounds.position += vec2{s.margin, s.margin};
                borderBounds.size -= 2.0 * vec2{s.margin, s.margin};
                if (borderBounds.size.x <= 0 || borderBounds.size.y <= 0)
                {
                    return rect {
                        {desiredBounds.position.x + (s.margin) / (s.margin + s.margin) * desiredBounds.size.x,
                         desiredBounds.position.y + (s.margin) / (s.margin + s.margin) * desiredBounds.size.y},
                        {0.0, 0.0}
                    };
                }
                // draw border
                g_nanovg.fill_rounded_rect(borderBounds, borderRadius, borderColor);
            }

            rect interiorBounds = desiredBounds;
            interiorBounds.position += vec2{s.margin + borderSize, s.margin + borderSize};
            interiorBounds.size -= 2.0 * vec2{s.margin + borderSize, s.margin + borderSize};
            if (interiorBounds.size.x <= 0 || interiorBounds.size.y <= 0)
            {
                return rect {
                    {desiredBounds.position.x + (s.margin + borderSize) / (s.margin + s.margin + 2 * borderSize) * desiredBounds.size.x,
                     desiredBounds.position.y + (s.margin + borderSize) / (s.margin + s.margin + 2 * borderSize) * desiredBounds.size.y},
                    {0.0, 0.0}
                };
            }
            corners interiorRadius = borderRadius;
            interiorRadius.topLeft -= borderSize;
            interiorRadius.topRight -= borderSize;
            interiorRadius.bottomRight -= borderSize;
            interiorRadius.bottomLeft -= borderSize;
            interiorRadius.topLeft = max(interiorRadius.topLeft, 0.0);
            interiorRadius.topRight = max(interiorRadius.topRight, 0.0);
            interiorRadius.bottomRight = max(interiorRadius.bottomRight, 0.0);
            interiorRadius.bottomLeft = max(interiorRadius.bottomLeft, 0.0);
            // draw interior background color
            g_nanovg.fill_rounded_rect(interiorBounds, interiorRadius, backgroundColor);

            // calculate internal bounds of widget content
            rect paddedBounds = desiredBounds;
            paddedBounds.position += vec2{s.margin + borderSize + s.padding, s.margin + borderSize + s.padding};
            paddedBounds.size -= 2.0 * vec2{s.margin + borderSize + s.padding, s.margin + borderSize + s.padding};
            if (paddedBounds.size.x <= 0 || paddedBounds.size.y <= 0)
            {
                return rect {
                    {desiredBounds.position.x + (s.margin + borderSize + s.padding) / (s.margin + s.margin + 2 * borderSize + 2 * s.padding) * desiredBounds.size.x,
                     desiredBounds.position.y + (s.margin + borderSize + s.padding) / (s.margin + s.margin + 2 * borderSize + 2 * s.padding) * desiredBounds.size.y},
                    {0.0, 0.0}
                };
            }

            return paddedBounds;
        }

        // vbox

        void widget_vbox::begin()
        {
            if (g_input.get_mouse_clicked(mouse_button::left, this->scissor))
            {
                this->focus_widget();
            } 
            else if (g_input.get_mouse_clicked(mouse_button::left))
            {
                this->unfocus_widget();
            }
        }

        render_bounds_callback_t widget_vbox::get_bounds_callback()
        {
            return [](void* ptr, const rect& desired) -> rect
            {
                auto& self = *static_cast<widget_vbox*>(ptr);
                rect retval =
                {
                    {self.bounds.position.x, self.bounds.position.y + self.internal_height - self.data()->vertical_scroll},
                    {self.bounds.size.x - self._style->scrollbarSize, desired.size.y}
                };
                self.internal_height += desired.size.y;
                return retval;
            };
        }

        void widget_vbox::middle()
        {
            // height of the group
            const auto height = bounds.size.y;
            // valid scrollable range  (0 to scrollable_height)
            const auto scrollable_height = internal_height - height;

            // style constants
            const auto scrollbar_width = _style->scrollbarSize;
            const auto min_scrollbar_height = _style->scrollbarMinLength;

            // scrollbar bg rect
            scrollbar_area = {{bounds.position.x + bounds.size.x - scrollbar_width, bounds.position.y}, {scrollbar_width, height}};

            // scrollbar rect
            double scrollbar_x = bounds.position.x  + bounds.size.x - scrollbar_width;

            double scrollbar_height = std::min(std::max(height*height / internal_height, min_scrollbar_height), height);
            double scrollbar_y = bounds.position.y + (height - scrollbar_height) * (data()->vertical_scroll / scrollable_height);

            scrollbar_grab_area = {{scrollbar_x, scrollbar_y}, {scrollbar_width, scrollbar_height}};
        }

        void widget_vbox::second_render() const
        {
            get_nanovg()->fill_rect(scrollbar_area, this->_style->scrollbarBackColor);

            corners radius = this->_style->scrollbarCornerRadius;

            if (data()->grabbing_scrollbar)
            {
                get_nanovg()->fill_rounded_rect(scrollbar_grab_area, radius, this->_style->scrollbarActiveColor);                
            }
            else
            {
                get_nanovg()->fill_rounded_rect(scrollbar_grab_area, radius, this->_style->scrollbarForeColor);
            }
            
        }

        void widget_vbox::end()
        {
            // height of the group
            const auto height = bounds.size.y;
            // valid scrollable range  (0 to scrollable_height)
            const  auto scrollable_height = internal_height - height;

            if (internal_height < height)
            {
                data()->vertical_scroll = 0.0;
            } 
            else
            {
                auto prev_mouse_down = g_input.get_mouse_button_state(mouse_button::left, input_frame::previous);
                auto current_mouse_down = g_input.get_mouse_button_state(mouse_button::left, input_frame::current);
                auto current_mouse_position = g_input.get_mouse_position(input_frame::current);

                // handle click and drag
                if (!data()->grabbing_scrollbar) 
                {
                    // mouse clicked
                    if (!prev_mouse_down && current_mouse_down)
                    {
                        // mouse in scrollbar grab area
                        if (point_inside_rect(current_mouse_position, scrollbar_grab_area))
                        {
                            data()->grabbing_scrollbar = true;
                            data()->grab_offset_y = current_mouse_position.y - scrollbar_grab_area.position.y;
                        }
                        // mouse in scrollbar area
                        else if(point_inside_rect(current_mouse_position, scrollbar_area))
                        {
                            data()->grabbing_scrollbar = true;
                            data()->grab_offset_y = scrollbar_grab_area.size.y / 2.0;
                            data()->vertical_scroll = (current_mouse_position.y - bounds.position.y - data()->grab_offset_y) / 
                                                    (bounds.size.y - scrollbar_grab_area.size.y) * scrollable_height;
                        }
                    } 
                }
                else
                {
                    // release scrollbar
                    if(!current_mouse_down)
                    {
                        data()->grabbing_scrollbar = false;
                    }
                    // drag scroll
                    else
                    {
                        data()->vertical_scroll = (current_mouse_position.y - bounds.position.y - data()->grab_offset_y) / 
                                                (bounds.size.y - scrollbar_grab_area.size.y) * scrollable_height;
                    } 
                }

                const auto scroll_step = this->_style->scrollbarStep;
                // handle scroll wheel
                if (!data()->grabbing_scrollbar && point_inside_rect(current_mouse_position, this->scissor))
                {
                    data()->vertical_scroll -= g_input.get_mouse_scroll(input_frame::current).y * scroll_step;
                }

                // handle keyboard input
                if (this->is_focused())
                {
                    for(auto keycode : g_input.get_keypresses())
                    {
                        switch(keycode)
                        {
                            case GLFW_KEY_PAGE_UP:
                                data()->vertical_scroll -= 5 * scroll_step;
                                break;
                            case GLFW_KEY_PAGE_DOWN:
                                data()->vertical_scroll += 5 * scroll_step;
                                break;
                            case GLFW_KEY_UP:
                                data()->vertical_scroll -= scroll_step;
                                break;
                            case GLFW_KEY_DOWN:
                                data()->vertical_scroll += scroll_step;
                                break;
                            case GLFW_KEY_HOME:
                                data()->vertical_scroll = 0;
                                break;
                            case GLFW_KEY_END:
                                data()->vertical_scroll = scrollable_height;
                                break;
                        }
                    }
                }

                // clamp it
                data()->vertical_scroll = clamp<double>(0, data()->vertical_scroll, scrollable_height);
            }            
        }

        // Hbox

        render_bounds_callback_t widget_hbox::get_bounds_callback()
        {
            return [](void* ptr, const rect&) -> rect
            {
                auto& self = *static_cast<widget_hbox*>(ptr);

                float w = self.widths[self._index++];

                rect retval =
                {
                    {self.bounds.position.x + self._horizontal_offset, self.bounds.position.y},
                    {w, self.bounds.size.y}
                };

                self._horizontal_offset += w;

                return retval;
            };
        }

        // Debug

        void widget_debug::begin()
        {
            auto current_position = g_input.get_mouse_position(input_frame::current);
            auto previous_position = g_input.get_mouse_position(input_frame::previous);
            if (point_inside_rect(current_position, scissor))
            {
                on_mouse_over(current_position);

                if (current_position != previous_position) 
                {
                    on_mouse_move(current_position);
                }
            }
        }
        void widget_debug::first_render() const
        {
            get_nanovg()->fill_rect(bounds, color::rgb(0,0,0));
            get_nanovg()->fill_rect(rect{bounds.position + vec2{1.0, 1.0}, {bounds.size - vec2{2.0, 2.0}}}, bg_color);
            // get_nanovg()->fill_rounded_rect((rectf)rect{bounds.position + vec2{1.0, 1.0}, {bounds.size - vec2{2.0, 2.0}}}, cornersf{10.0f}, color::rgb(0x77, 0x77, 0x77));
        }

        // Label

        void widget_label::first_render() const
        {
            g_nanovg.draw_text(
                bounds,
                data()->font,
                data()->text_size,
                data()->text_color,
                data()->text_halign,
                data()->text_valign,
                data()->text);
        }

        // Button

        void widget_button::begin()
        {
            if (g_input.get_mouse_clicked(mouse_button::left, this->scissor))
            {
                this->focus_widget();
            } 
            else if (g_input.get_mouse_clicked(mouse_button::left))
            {
                this->unfocus_widget();
            }            
        }

        void widget_button::first_render() const
        {
            auto current_mouse_position = g_input.get_mouse_position(input_frame::current);
            if(point_inside_rect(current_mouse_position, scissor))
            {
                if (!g_input.get_mouse_button_state(mouse_button::left, input_frame::current))
                {
                    g_nanovg.fill_rect(bounds, g_style.button.background_hover_color);
                }
                else
                {
                    g_nanovg.fill_rect(bounds, g_style.button.background_active_color);
                }
            }
            else
            {
                g_nanovg.fill_rect(bounds, g_style.button.background_color);
            }
            
            g_nanovg.draw_text(
                bounds,
                g_style.button.font_face,
                g_style.button.font_size,
                g_style.button.font_color,
                halign::center,
                valign::middle,
                data()->text);
        }

        void widget_button::end()
        {
            if (g_input.get_mouse_clicked(mouse_button::left, this->scissor))
            {
                this->on_click();
            }    
        }
    }
}