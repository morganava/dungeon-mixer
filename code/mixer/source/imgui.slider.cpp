#include "imgui.hpp"
#include "input.hpp"

namespace tabletopify
{
    namespace imgui
    {
        void widget_slider::middle_update_impl()
        {
            auto bounds = this->_layout.child_bounds();
            const auto sliderSize = this->style().sliderHandleSize;

            auto position_to_val = [&](double pos, double minVal, double maxVal) -> double
            {
                return clamp(0.0, (pos - (bounds.left() + sliderSize.x / 2.0)) / (bounds.width() - sliderSize.x), 1.0) * (maxVal - minVal);
            };

            double startVal = data()->currentVal;

            for(auto& evt : g_input.get_input_events())
            {
                switch (evt.type())
                {
                    case input_event_type::mouse_drag_begin:
                    {
                        this->unfocus_widget();
                        if (point_inside_rect(evt.position(), this->scissor()))
                        {
                            if (point_inside_rect(evt.position(), bounds) ||
                                point_inside_rect(evt.position(), this->slider_rect(bounds)))
                            {
                                this->focus_widget();
                                auto x = evt.position().x;
                                data()->currentVal = position_to_val(x, data()->minVal, data()->maxVal);
                            }
                        }
                    }
                    break;
                    case input_event_type::mouse_drag:
                    case input_event_type::mouse_drag_end:
                    {
                        if (this->is_focused())
                        {
                            auto x = evt.position().x;
                            data()->currentVal = position_to_val(x, data()->minVal, data()->maxVal);
                        }
                    }
                    break;
                    case input_event_type::mouse_single_click:
                    {
                        if (point_inside_rect(evt.position(), this->scissor()) &&
                            point_inside_rect(evt.position(), bounds))
                        {
                            auto x = evt.position().x;
                            data()->currentVal = position_to_val(x, data()->minVal, data()->maxVal);
                            this->focus_widget();
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

            if (data()->currentVal != startVal) {
                this->on_change(data()->currentVal);
            }
        }

        void widget_slider::first_render_impl() const
        {
            const auto& style = this->style();
            const auto bounds = this->_layout.child_bounds();

            // draw background

            g_nanovg.fill_rounded_rect(
                bounds,
                style.sliderBackgroundCornerRadius,
                style.sliderBackgroundColor);

            g_nanovg.stroke_rounded_rect(
                bounds,
                style.sliderBackgroundCornerRadius,
                style.sliderBackgroundBorderSize,
                style.sliderBackgroundBorderColor);

            // draw slider
            const auto sliderHandleRect = this->slider_rect(bounds);


            g_nanovg.fill_rounded_rect(
                sliderHandleRect,
                style.sliderHandleCornerRadius,
                style.sliderHandleColor);

            g_nanovg.stroke_rounded_rect(
                sliderHandleRect,
                style.sliderHandleCornerRadius,
                style.sliderHandleBorderSize,
                style.sliderHandleBorderColor);
        }

        rect widget_slider::slider_rect(const rect& bounds) const
        {
            const auto& style = this->style();
            auto val_to_position = [](double val, double minVal, double maxVal, double left, double right) -> double
            {
                return (val - minVal) / (maxVal - minVal) * (right - left) + left;
            };

            double sliderCenterX = val_to_position(
                data()->currentVal,
                data()->minVal,
                data()->maxVal,
                bounds.left() + style.sliderHandleSize.x / 2.0,
                bounds.right() - style.sliderHandleSize.x / 2.0);

            return rect
            {
                {
                    sliderCenterX - (style.sliderHandleSize.x / 2.0),
                    bounds.top() + (bounds.height() / 2.0) - (style.sliderHandleSize.y / 2.0)
                },
                style.sliderHandleSize
            };
        }
    }
}