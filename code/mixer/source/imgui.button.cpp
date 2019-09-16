#include "imgui.hpp"
#include "input.hpp"

namespace tabletopify
{
    namespace imgui
    {
        // Button

        element_state_t widget_button::resolve_element_state_impl() const
        {
            const auto mouse_position = g_input.get_mouse_position(input_frame::current);

            if (point_inside_rect(
                    mouse_position,
                    this->scissor()) &&
                point_inside_rounded_rect(
                    mouse_position,
                    this->_layout.border_outer_bounds(),
                    this->_layout.borderRadius))
            {
                return element_state::hover;
            }
            return element_state::none;
        }

        void widget_button::first_update_impl()
        {
            for(auto evt : g_input.get_input_events())
            {
                if (evt.type() == input_event_type::mouse_single_click)
                {
                    if (point_inside_rect(evt.position(), this->scissor()) &&
                        point_inside_rounded_rect(evt.position(),
                                                  this->_layout.border_outer_bounds(),
                                                  this->_layout.borderRadius))
                    {
                        this->focus_widget();
                        this->_clicked = true;
                    }
                    else
                    {
                        this->unfocus_widget();
                    }
                }
            }
        }

        void widget_button::first_render_impl() const
        {
            g_nanovg.draw_text(
                this->_layout.inner_bounds(),
                0.0f,
                this->style().fontFamily,
                this->style().fontSize,
                this->style().fontColor,
                this->style().textHalign,
                this->style().textValign,
                data()->text);
        }

        void widget_button::last_update_impl()
        {
            if (this->_clicked)
            {
                this->on_click();
            }
        }
    }
}