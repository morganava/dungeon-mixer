#include "imgui.hpp"
#include "input.hpp"

namespace dm
{
    namespace imgui
    {
        struct widget_range_slider_track : public widget<widget_range_slider_track, element_type::range_slider_track>
        {
            using widget::widget;
        };

        struct widget_range_slider_handle : public widget<widget_range_slider_handle, element_type::range_slider_handle>
        {
            using widget::widget;

            void first_update_impl();
            element_state_t resolve_element_state_impl();

            widget_range_slider* _slider = nullptr;
            range_slider_handle_t _handle = range_slider_handle::invalid;
        };

        void widget_range_slider_handle::first_update_impl()
        {
            MX_ASSERT(_handle == range_slider_handle::low ||
                      _handle == range_slider_handle::high);

            const auto sliderBounds = _slider->_layout.child_bounds();
            auto& lowWidth = _slider->data()->lowWidth;
            auto& highWidth = _slider->data()->highWidth;

            if (_handle == range_slider_handle::low)
            {
                lowWidth = this->_layout.outerBounds.width();
            }
            else
            {
                highWidth = this->_layout.outerBounds.width();
            }

            const auto handleBounds = this->_layout.border_outer_bounds();

            const auto left = sliderBounds.left() + lowWidth;
            const auto right = sliderBounds.right() - highWidth;
            const auto minVal = _slider->data()->minValue;
            const auto maxVal = _slider->data()->maxValue;
            const auto& lowValue = _slider->data()->lowValue;
            const auto& highValue = _slider->data()->highValue;


            auto& value = (_handle == range_slider_handle::low) ? _slider->data()->lowValue : _slider->data()->highValue;

            auto position_to_val = [&](double pos) -> double
            {
                auto t = (pos - left) / (right - left);
                auto retval = lerp(minVal, maxVal, t);

                // clamp the value between edges and the other slider
                if (_handle == range_slider_handle::low)
                {
                    retval = clamp(minVal, retval, highValue);
                }
                else
                {
                    retval = clamp(lowValue, retval, maxVal);
                }

                return retval;
            };

            for(auto& evt : g_input->get_input_events())
            {
                switch (evt.type())
                {
                    case input_event_type::mouse_down:
                    case input_event_type::mouse_drag_begin:
                    {
                        if (point_inside_rect(evt.position(), _slider->scissor()))
                        {
                            if (point_inside_rect(evt.position(), handleBounds))
                            {
                                _slider->data()->focusedHandle = _handle;
                                _slider->focus_widget();

                                auto x = evt.position().x;
                                _slider->data()->mouseDelta =
                                    (_handle == range_slider_handle::low ?
                                      handleBounds.right() :
                                      handleBounds.left()) - x;
                            }
                        }
                        else
                        {
                            _slider->unfocus_widget();
                            _slider->data()->focusedHandle = range_slider_handle::invalid;
                        }
                    }
                    break;
                    case input_event_type::mouse_drag:
                    {
                        if (_slider->data()->focusedHandle == _handle)
                        {
                            auto x = evt.position().x + _slider->data()->mouseDelta;
                            value = position_to_val(x);
                        }
                    }
                    break;
                    case input_event_type::mouse_up:
                    case input_event_type::mouse_drag_end:
                    {
                        _slider->unfocus_widget();
                        _slider->data()->focusedHandle = range_slider_handle::invalid;
                        _slider->data()->mouseDelta = 0.0;
                    }
                    default:
                    break;
                }
            }
        }

        element_state_t widget_range_slider_handle::resolve_element_state_impl()
        {
            auto state = widget::resolve_element_state_impl();
            if (_slider->data()->focusedHandle == _handle)
            {
                state |= element_state::active;
            }
            else if (_slider->data()->focusedHandle != range_slider_handle::invalid)
            {
                state &= ~(element_state::hover | element_state::active);
            }

            return state;
        }

        #define mxRangeSliderTrack(...) MX_WIDGET_CODEGEN(widget_range_slider_track, nullptr, __VA_ARGS__)
        #define mxRangeSliderHandle(...) MX_WIDGET_CODEGEN(widget_range_slider_handle, nullptr, __VA_ARGS__)

        //
        // Range Slider
        //

        rect widget_range_slider::bounds_callback_impl(rect desiredBounds)
        {
            constexpr size_t SLIDER_TRACK_ID = 0;
            constexpr size_t SLIDER_LOW_HANDLE_ID = 1;
            constexpr size_t SLIDER_HIGH_HANDLE_ID = 2;

            MX_ASSERT(_childCounter == SLIDER_TRACK_ID ||
                      _childCounter == SLIDER_LOW_HANDLE_ID ||
                      _childCounter == SLIDER_HIGH_HANDLE_ID);

            const auto sliderBounds = _layout.child_bounds();
            if (_childCounter == SLIDER_TRACK_ID)
            {
                return sliderBounds;
            }

            const auto& min = data()->minValue;
            const auto& max = data()->maxValue;
            const auto left = sliderBounds.left() + data()->lowWidth;
            const auto right = sliderBounds.right() - data()->highWidth;

            const auto& handleSize = desiredBounds.size;

            // low handle
            if (_childCounter == SLIDER_LOW_HANDLE_ID)
            {
                const auto& low = data()->lowValue;
                const auto t = (low - min) / (max - min);
                const auto rightEdge = lerp(left, right, t);

                return
                {
                    {
                        rightEdge - handleSize.x,
                        sliderBounds.center().y - (handleSize.y / 2.0)
                    },
                    handleSize,
                };
            }
            // high handle
            else
            {
                const auto& high = data()->highValue;
                const auto t = (high - min) / (max - min);
                const auto leftEdge = lerp(left, right, t);

                return
                {
                    {
                        leftEdge,
                        sliderBounds.center().y - (handleSize.y / 2.0)
                    },
                    handleSize,
                };
            }
        }

        void widget_range_slider::last_update_impl()
        {
            const auto startLow = data()->lowValue;
            const auto startHigh = data()->highValue;

            const auto& tags = this->tags;

            mxRangeSliderTrack(self.tags = tags);
            auto slider = this;
            // low handle
            mxRangeSliderHandle(self._slider = slider; self._handle = range_slider_handle::low; self.tags = tags.concat({"low"}));
            // high handle
            mxRangeSliderHandle(self._slider = slider; self._handle = range_slider_handle::high; self.tags = tags.concat({"high"}));

            if (startLow != data()->lowValue ||
                startHigh != data()->highValue)
            {
                this->on_change(data()->lowValue, data()->highValue);
            }
        }

        double widget_range_slider::lowValue() const
        {
            return this->data()->minValue;
        }

        double widget_range_slider::highValue() const
        {
            return this->data()->maxValue;
        }
    }
}