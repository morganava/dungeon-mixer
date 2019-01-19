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

        void elem_group::initialize()
        {
            // constexpr double scrollStep = 10.0;
            // group_data->vertical_scroll += get_input()->get_mouse_scroll(false).y * scrollStep;
            // // group_data->vertical_scroll = clamp<double>(0, group_data->vertical_scroll, _vertical_offset);
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
                        {self.bounds.position.x, self.bounds.position.y + self._vertical_offset - self.group_data->vertical_scroll},
                        {self.bounds.size.x - scrollbar_width, desired.size.y}
                    };
                    self._vertical_offset += desired.size.y;
                    return retval;
                },
                this
            };
        }

        void elem_group::second_render() const
        {
            // height of the group
            auto height = bounds.size.y;
            // height of all of the child widgets
            auto internal_height = _vertical_offset;
            // valid scrollable range  (0 to scrollable_height)
            auto scrollable_height = internal_height - bounds.size.y;

            // scrollbar bg rect
            rect scroll_bg_bounds = {{bounds.size.x - scrollbar_width, 0.0}, {scrollbar_width, height}};
            get_nanovg()->fill_rect(scroll_bg_bounds, nvgRGB(96,96,96));

            // scrollbar rect
            double scrollbar_x = bounds.size.x - scrollbar_width;

            constexpr double min_scrollbar_height = 150.0;
            double scrollbar_height = std::min(std::max(height*height / internal_height, min_scrollbar_height), height);
            double scrollbar_y = (height - scrollbar_height) * (group_data->vertical_scroll / scrollable_height);

            rect scrollbar_bounds = {{scrollbar_x + 2.0, scrollbar_y + 2.0},{scrollbar_width - 4.0, scrollbar_height - 4.0}};
            get_nanovg()->fill_rect(scrollbar_bounds, nvgRGB(192,192,192));

            if (internal_height < height)
            {
                group_data->vertical_scroll = 0.0;
            } else
            {
                constexpr double scroll_step = 10.0;
                group_data->vertical_scroll -= get_input()->get_mouse_scroll(false).y * scroll_step;
                group_data->vertical_scroll = clamp<double>(0, group_data->vertical_scroll, scrollable_height);
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

        void elem_debug::initialize()
        {
            auto* input = get_input();
            auto current_position = input->get_mouse_position(false);
            auto previous_position = input->get_mouse_position(true);
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