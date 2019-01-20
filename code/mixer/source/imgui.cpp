#include "singleton.hpp"
#include "geometry.hpp"
#include "utilities.hpp"
#include "utf8_string.hpp"
#include "input.hpp"
#include "hash.hpp"
#include "nanovg.hpp"
#include "imgui.hpp" 

namespace tabletopify
{
    namespace imgui
    {
        widget_id_t next_widget_id()
        {
            static widget_id_t nextId = 0;
            return nextId++;
        }

        // Group

        void elem_group::begin()
        {

        }

        bounds_callback elem_group::get_bounds_callback()
        {
            return
            {
                [](void* ptr, const rect& desired) -> rect
                {
                    auto& self = *static_cast<elem_group*>(ptr);
                    rect retval =
                    {
                        {self.bounds.position.x, self.bounds.position.y + self.internal_height - self.data->vertical_scroll},
                        {self.bounds.size.x - scrollbar_width, desired.size.y}
                    };
                    self.internal_height += desired.size.y;
                    return retval;
                },
                this
            };
        }

        void elem_group::middle()
        {
            // height of the group
            const auto height = bounds.size.y;
            // valid scrollable range  (0 to scrollable_height)
            const auto scrollable_height = internal_height - height;

            // scrollbar rect
            double scrollbar_x = bounds.size.x - scrollbar_width;

            double scrollbar_height = std::min(std::max(height*height / internal_height, min_scrollbar_height), height);
            double scrollbar_y = (height - scrollbar_height) * (data->vertical_scroll / scrollable_height);

            scrollbar_grab_area = {{scrollbar_x + 2.0, scrollbar_y + 2.0},{scrollbar_width - 4.0, scrollbar_height - 4.0}};
        }

        void elem_group::second_render() const
        {
            // height of the group
            const auto height = bounds.size.y;

            // scrollbar bg rect
            rect scroll_bg_bounds = {{bounds.size.x - scrollbar_width, 0.0}, {scrollbar_width, height}};
            get_nanovg()->fill_rect(scroll_bg_bounds, nvgRGB(96,96,96));

            if (data->grabbing_scrollbar)
            {
                get_nanovg()->fill_rect(scrollbar_grab_area, nvgRGB(192,96,0));                
            }
            else
            {
                get_nanovg()->fill_rect(scrollbar_grab_area, nvgRGB(192,192,192));
            }
            
        }

        void elem_group::end()
        {
            // height of the group
            const auto height = bounds.size.y;
            // valid scrollable range  (0 to scrollable_height)
            const  auto scrollable_height = internal_height - height;

            if (internal_height < height)
            {
                data->vertical_scroll = 0.0;
            } 
            else
            {
                auto* input = get_input();

                auto prev_mouse_down = input->get_mouse_button_state(mouse_button::left, input_frame::previous);
                auto current_mouse_down = input->get_mouse_button_state(mouse_button::left, input_frame::current);
                auto prev_mouse_position = input->get_mouse_position(input_frame::previous);
                auto current_mouse_position = input->get_mouse_position(input_frame::current);

                // handle click and drag
                if (!data->grabbing_scrollbar) 
                {
                    // mouse clicked
                    if (!prev_mouse_down && current_mouse_down)
                    {
                        // mouse in scrollbar area
                        if (point_inside_rect(current_mouse_position, scrollbar_grab_area))
                        {
                            data->grabbing_scrollbar = true;
                        }
                    } 
                }
                else
                {
                    if(!current_mouse_down)
                    {
                        data->grabbing_scrollbar = false;
                    }
                    else if (current_mouse_position.y >= bounds.position.y &&
                             current_mouse_position.y <= bounds.position.y + bounds.size.y)
                    {
                        double y_delta = current_mouse_position.y - prev_mouse_position.y;
                        data->vertical_scroll += y_delta / (height - scrollbar_grab_area.size.y) * scrollable_height;
                    }
                }

                // handle scroll wheel
                if (!data->grabbing_scrollbar)
                {
                    data->vertical_scroll -= input->get_mouse_scroll(input_frame::current).y * scroll_step;
                }

                // clamp it
                data->vertical_scroll = clamp<double>(0, data->vertical_scroll, scrollable_height);
            }            
        }

        // Static Row

        bounds_callback elem_static_row::get_bounds_callback()
        {
            return
            {
                [](void* ptr, const rect&) -> rect
                {
                    auto& self = *static_cast<elem_static_row*>(ptr);
                    float w = self._widths.front();
                    self._widths.erase(self._widths.begin());

                    rect retval =
                    {
                        {self.bounds.position.x + self._horizontal_offset, self.bounds.position.y},
                        {w, self.bounds.size.y}
                    };

                    self._horizontal_offset += w;

                    return retval;
                },
                this
            };
        }

        // Debug

        void elem_debug::begin()
        {
            auto* input = get_input();
            auto current_position = input->get_mouse_position(input_frame::current);
            auto previous_position = input->get_mouse_position(input_frame::previous);
            if (point_inside_rect(current_position, bounds))
            {
                on_mouse_over(current_position);

                if (current_position != previous_position) 
                {
                    on_mouse_move(current_position);
                }
            }
        }
        void elem_debug::first_render() const
        {
            get_nanovg()->fill_rect(bounds, nvgRGB(0,0,0));
            get_nanovg()->fill_rect(rect{bounds.position + vec2{1.0, 1.0}, {bounds.size - vec2{2.0, 2.0}}}, bg_color);
        }

        bounds_callback elem_debug::get_bounds_callback()
        {
            return {};
        }
    }
}