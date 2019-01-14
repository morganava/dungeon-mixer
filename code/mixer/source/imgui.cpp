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

        void elem_group::pre_render()
        {

        }

        void elem_group::render() const
        {

        }

        void elem_group::post_render()
        {
            _ctx.push_bounds(
            {
                [](void* ptr, const rect& desired) -> rect
                {
                    elem_group* eg = reinterpret_cast<elem_group*>(ptr);
                    rect retval =
                    {
                        {eg->bounds.position.x, eg->bounds.position.y + eg->_vertical_offset},
                        {eg->bounds.size.x, desired.size.y}
                    };
                    eg->_vertical_offset += desired.size.y;
                    return retval;
                },
                this
            });
        }

        // Static Row

        void elem_static_row::pre_render()
        {

        }

        void elem_static_row::render() const
        {

        }

        void elem_static_row::post_render()
        {
            _ctx.push_bounds(
            {
                [](void* ptr, const rect&) -> rect
                {
                    elem_static_row* esr = static_cast<elem_static_row*>(ptr);
                    float w = esr->_widths.front();
                    esr->_widths.erase(esr->_widths.begin());

                    rect retval =
                    {
                        {esr->bounds.position.x + esr->_horizontal_offset, esr->bounds.position.y},
                        {w, esr->bounds.size.y}
                    };

                    esr->_horizontal_offset += w;

                    return retval;
                },
                this
            });
        }

        // Debug

        void elem_debug::pre_render()
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

        void elem_debug::post_render()
        {
            _ctx.push_bounds({});
        }
    }
}