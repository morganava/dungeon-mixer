#include "imgui.hpp"
#include "input.hpp"

namespace tabletopify
{
    namespace imgui
    {
        struct widget_slider_track : public widget<widget_slider_track, element_type::slider_track>
        {
            using widget::widget;
        };

        struct widget_slider_handle : public widget<widget_slider_handle, element_type::slider_handle>
        {
            using widget::widget;

            void middle_update_impl();

            widget_slider* _slider = nullptr;
        };

        void widget_slider_handle::middle_update_impl()
        {
            const auto sliderBounds = _slider->_layout.child_bounds();
            const auto handleWidth = this->_layout.outerBounds.width();
            const auto handleBounds = this->_layout.border_outer_bounds();
            const auto startVal = _slider->data()->currentVal;
            const auto minVal = _slider->data()->minVal;
            const auto maxVal = _slider->data()->maxVal;

            auto position_to_val = [&](double pos) -> double
            {
                return clamp(
                    0.0,
                    (pos - (sliderBounds.left() + handleWidth / 2.0)) / (sliderBounds.width() - handleWidth),
                    1.0) * (maxVal - minVal);
            };

            for(auto& evt : g_input.get_input_events())
            {
                switch (evt.type())
                {
                    case input_event_type::mouse_drag_begin:
                    {
                        _slider->unfocus_widget();
                        if (point_inside_rect(evt.position(), _slider->scissor()))
                        {
                            if (point_inside_rect(evt.position(), sliderBounds) ||
                                point_inside_rect(evt.position(), handleBounds))
                            {
                                _slider->focus_widget();
                                auto x = evt.position().x;
                                _slider->data()->currentVal = position_to_val(x);
                            }
                        }
                    }
                    break;
                    case input_event_type::mouse_drag:
                    case input_event_type::mouse_drag_end:
                    {
                        if (_slider->is_focused())
                        {
                            auto x = evt.position().x;
                            _slider->data()->currentVal = position_to_val(x);
                        }
                    }
                    break;
                    case input_event_type::mouse_single_click:
                    {
                        if (point_inside_rect(evt.position(), _slider->scissor()) &&
                            point_inside_rect(evt.position(), sliderBounds))
                        {
                            auto x = evt.position().x;
                            _slider->data()->currentVal = position_to_val(x);
                            _slider->focus_widget();
                        }
                        else
                        {
                            _slider->unfocus_widget();
                        }
                    }
                    break;
                    default:
                    break;
                }
            }

            if (startVal != _slider->data()->currentVal) {
                _slider->on_change(_slider->data()->currentVal);
            }
        }

        #define mxSliderTrack() MX_WIDGET_CODEGEN(widget_slider_track, nullptr)
        #define mxSliderHandle(...) MX_WIDGET_CODEGEN(widget_slider_handle, nullptr, __VA_ARGS__)

        //
        // Slider
        //

        rect widget_slider::bounds_callback(widget_base* parent, const rect& desired)
        {
            constexpr size_t SLIDER_TRACK_ID = 0;
            constexpr size_t SLIDER_HANDLE_ID = 1;

            auto& self = *static_cast<widget_slider*>(parent);

            switch(self._childCounter) {
                case SLIDER_TRACK_ID:
                    // fill entire child space
                    return self._layout.child_bounds();
                case SLIDER_HANDLE_ID:
                    // desired will already have the size, just need to set the position
                    vec2 center = self.handle_center(desired.width());
                    vec2 position = center - (desired.size / 2.0);

                    return rect(position, desired.size);
            }
            return {};
        }

        void widget_slider::last_update_impl()
        {
            mxSliderTrack();
            auto slider = this;
            mxSliderHandle(self._slider = slider);
        }

        vec2 widget_slider::handle_center(double sliderWidth) const
        {
            const auto val = data()->currentVal;
            const auto minVal = data()->minVal;
            const auto maxVal = data()->maxVal;

            const auto bounds = this->_layout.child_bounds();
            const auto left = bounds.left() + sliderWidth / 2.0;
            const auto right = bounds.right() - sliderWidth / 2.0;

            double x = (val -  minVal) / (maxVal - minVal) * (right - left) + left;
            double y = bounds.top() + bounds.height() / 2.0;

            return {x, y};

        }
    }
}