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
        constexpr stylesheet& self_style = g_context.style;

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

        void widget_base::calc_render_bounds_impl(render_bounds(*func)(const rect&, void*))
        { 
            const auto& rb = g_context.peek_render_bounds();

            bounds = rb.eval(bounds);
            const auto& parent_scissor = rb.scissor;

            // calc the scissor geometry
            scissor = get_intersecting_rect(parent_scissor, bounds);
            get_nanovg()->set_scissor(scissor);

            // push info for child render bounds calculations
            g_context.push_render_bounds(func(scissor,this));
        }

        // vbox

        void widget_vbox::begin()
        {
            if (get_input()->get_mouse_clicked(mouse_button::left, this->scissor))
            {
                this->focus_widget();
            } 
            else if (get_input()->get_mouse_clicked(mouse_button::left))
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
                    {self.bounds.size.x - self_style.vbox.scrollbar_width, desired.size.y}
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
            const auto scrollbar_width = self_style.vbox.scrollbar_width;
            const auto min_scrollbar_height = self_style.vbox.min_scrollbar_height;

            // scrollbar bg rect
            scrollbar_area = {{bounds.position.x + bounds.size.x - scrollbar_width, bounds.position.y}, {scrollbar_width, height}};

            // scrollbar rect
            double scrollbar_x = bounds.position.x  + bounds.size.x - scrollbar_width;

            double scrollbar_height = std::min(std::max(height*height / internal_height, min_scrollbar_height), height);
            double scrollbar_y = bounds.position.y + (height - scrollbar_height) * (data()->vertical_scroll / scrollable_height);

            scrollbar_grab_area = {{scrollbar_x + 2.0, scrollbar_y + 2.0},{scrollbar_width - 4.0, scrollbar_height - 4.0}};
        }

        void widget_vbox::second_render() const
        {
            get_nanovg()->fill_rect(scrollbar_area, nvgRGB(96,96,96));

            if (data()->grabbing_scrollbar)
            {
                get_nanovg()->fill_rect(scrollbar_grab_area, nvgRGB(192,96,0));                
            }
            else
            {
                get_nanovg()->fill_rect(scrollbar_grab_area, nvgRGB(192,192,192));
            }
            
        }

        void widget_vbox::end()
        {
            // height of the group
            const auto height = bounds.size.y;
            // valid scrollable range  (0 to scrollable_height)
            const  auto scrollable_height = internal_height - height;

            auto* input = get_input();

            if (internal_height < height)
            {
                data()->vertical_scroll = 0.0;
            } 
            else
            {
                auto prev_mouse_down = input->get_mouse_button_state(mouse_button::left, input_frame::previous);
                auto current_mouse_down = input->get_mouse_button_state(mouse_button::left, input_frame::current);
                auto current_mouse_position = input->get_mouse_position(input_frame::current);

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

                auto scroll_step = self_style.vbox.scroll_step;

                // handle scroll wheel
                if (!data()->grabbing_scrollbar && point_inside_rect(current_mouse_position, this->scissor))
                {
                    data()->vertical_scroll -= input->get_mouse_scroll(input_frame::current).y * scroll_step;
                }

                // handle keyboard input
                if (this->is_focused())
                {
                    for(auto keycode : input->get_keypresses())
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
                float w = self._widths.front();
                self._widths.erase(self._widths.begin());

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
            auto* input = get_input();
            auto current_position = input->get_mouse_position(input_frame::current);
            auto previous_position = input->get_mouse_position(input_frame::previous);
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
            get_nanovg()->fill_rect(bounds, nvgRGB(0,0,0));
            get_nanovg()->fill_rect(rect{bounds.position + vec2{1.0, 1.0}, {bounds.size - vec2{2.0, 2.0}}}, bg_color);
        }

        // Label


    }
}