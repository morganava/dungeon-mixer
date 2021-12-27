#include "../imgui.hpp"
#include "../input.hpp"

namespace dm
{
    namespace imgui
    {
        // Button

        void widget_button::handle_input_impl()
        {
            for(auto evt : g_input->get_input_events())
            {
                switch(evt.type())
                {
                    case input_event_type::mouse_single_click:
                    case input_event_type::mouse_double_click:
                    {
                        if (point_inside_rect(evt.position(), this->scissor()) &&
                            point_inside_rounded_rect(evt.position(),
                                                      this->_layout.border_outer_bounds(),
                                                      this->_layout.borderRadius))
                        {
                            this->focus_widget();
                            if (evt.type() == input_event_type::mouse_single_click)
                            {
                                this->_clicked = true;
                            }
                            else
                            {
                                this->_double_clicked = true;
                            }

                        }
                        else
                        {
                            this->unfocus_widget();
                        }
                    }
                    break;
                    default:
                    break;
                }
            }
        }

        void widget_button::first_render_impl() const
        {
            g_nanovg->draw_text(
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
            if (this->_double_clicked)
            {
                this->on_double_click();
            }
        }
    }
}