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

            void first_update_impl();
            element_state_t resolve_element_state_impl();

            widget_slider* _slider = nullptr;
        };

        void widget_slider_handle::first_update_impl()
        {
            const auto sliderBounds = _slider->_layout.child_bounds();
            const auto handleWidth = this->_layout.outerBounds.width();
            const auto handleBounds = this->_layout.border_outer_bounds();
            const auto startVal = _slider->data()->value;
            const auto minVal = _slider->data()->minVal;
            const auto maxVal = _slider->data()->maxVal;

            auto position_to_val = [&](double pos) -> double
            {
                return clamp(
                    0.0,
                    (pos - (sliderBounds.left() + handleWidth / 2.0)) / (sliderBounds.width() - handleWidth),
                    1.0) * (maxVal - minVal) + minVal;
            };

            for(auto& evt : g_input.get_input_events())
            {
                switch (evt.type())
                {
                    case input_event_type::mouse_down:
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
                                _slider->data()->value = position_to_val(x);
                            }
                        }
                    }
                    break;
                    case input_event_type::mouse_drag:
                    {
                        if (_slider->is_focused())
                        {
                            auto x = evt.position().x;
                            _slider->data()->value = position_to_val(x);
                        }
                    }
                    break;
                    case input_event_type::mouse_drag_end:
                    {
                        _slider->unfocus_widget();
                    }
                    break;
                    case input_event_type::mouse_single_click:
                    {
                        if (point_inside_rect(evt.position(), _slider->scissor()) &&
                            point_inside_rect(evt.position(), sliderBounds))
                        {
                            auto x = evt.position().x;
                            _slider->data()->value = position_to_val(x);
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

            if (startVal != _slider->data()->value) {
                _slider->on_change(_slider->data()->value);
            }
        }

        element_state_t widget_slider_handle::resolve_element_state_impl()
        {
            if (_slider->is_focused()) {
                return element_state::active;
            }

            return widget_base::resolve_element_state_impl();
        }

        #define mxSliderTrack(...) MX_WIDGET_CODEGEN(widget_slider_track, nullptr, __VA_ARGS__)
        #define mxSliderHandle(...) MX_WIDGET_CODEGEN(widget_slider_handle, nullptr, __VA_ARGS__)

        //
        // Slider
        //

        rect widget_slider::bounds_callback_impl(rect desired)
        {
            constexpr size_t SLIDER_TRACK_ID = 0;
            constexpr size_t SLIDER_HANDLE_ID = 1;

            auto& self = *this;

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
            const auto tags = this->tags;
            mxSliderTrack(self.tags = tags);
            auto slider = this;
            mxSliderHandle(self._slider = slider; self.tags = tags);
        }

        vec2 widget_slider::handle_center(double sliderWidth) const
        {
            const auto val = data()->value;
            const auto minVal = data()->minVal;
            const auto maxVal = data()->maxVal;

            const auto bounds = this->_layout.child_bounds();
            const auto left = bounds.left() + sliderWidth / 2.0;
            const auto right = bounds.right() - sliderWidth / 2.0;

            double x = (val -  minVal) / (maxVal - minVal) * (right - left) + left;
            double y = bounds.top() + bounds.height() / 2.0;

            return {x, y};
        }

        double widget_slider::value() const
        {
            return this->data()->value;
        }
    }
}