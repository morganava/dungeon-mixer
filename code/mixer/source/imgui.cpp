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
        // Group

        void elem_group::update_child_bounds()
        {
            _ctx.push_bounds(
            {
                [](void* ptr, const rect& desired) -> rect
                {
                    auto& self = *static_cast<elem_group*>(ptr);
                    rect retval =
                    {
                        {self.bounds.position.x, self.bounds.position.y + self._vertical_offset},
                        {self.bounds.size.x, desired.size.y}
                    };
                    self._vertical_offset += desired.size.y;
                    return retval;
                },
                this
            });
        }

        // Static Row

        void elem_static_row::update_child_bounds()
        {
            _ctx.push_bounds(
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
            });
        }

        // Debug

        void elem_debug::handle_input()
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
        void elem_debug::render() const
        {
            get_nanovg()->fill_rect(bounds, nvgRGB(0,0,0));
            get_nanovg()->fill_rect(rect{bounds.position + vec2{1.0, 1.0}, {bounds.size - vec2{2.0, 2.0}}}, bg_color);
        }

        void elem_debug::update_child_bounds()
        {
            _ctx.push_bounds({});
        }
    }
}