#include "../imgui.hpp"
#include "../input.hpp"

namespace dm
{
    namespace imgui
    {
        // Checkbox

        element_state_t widget_checkbox::resolve_element_state_impl()
        {
            element_state_t retval = widget_base::resolve_element_state_impl();
            if (data()->checked) {
                retval = retval | element_state::checked;
            }
            return retval;
        }

        void widget_checkbox::first_update_impl()
        {
            for(auto evt : g_input->get_input_events())
            {
                if (evt.type() == input_event_type::mouse_single_click ||
                    evt.type() == input_event_type::mouse_double_click ||
                    evt.type() == input_event_type::mouse_triple_click)
                {
                    if (point_inside_rect(evt.position(), this->scissor()) &&
                        point_inside_rounded_rect(evt.position(),
                                                  this->_layout.border_outer_bounds(),
                                                  this->_layout.borderRadius))
                    {
                        this->focus_widget();
                        this->_clicked = true;
                        data()->checked = !data()->checked;
                    }
                    else
                    {
                        this->unfocus_widget();
                    }
                }
            }
        }

        void widget_checkbox::last_update_impl()
        {
            if (this->_clicked)
            {
                this->on_change(data()->checked);
            }
        }
    }
}