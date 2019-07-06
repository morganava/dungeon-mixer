#include "singleton.hpp"
#include "geometry.hpp"
#include "utilities.hpp"
#include "utf8_string.hpp"
#include "input.hpp"
#include "hash.hpp"
#include "nanovg.hpp"
#include "imgui.hpp"
#include "ui.hpp"

namespace tabletopify
{
    namespace imgui
    {
        widget_id_t next_widget_id()
        {
            static widget_id_t nextId = 0;
            return nextId++;
        }

        // render_bounds

        rect render_bounds::eval(const rect& desired) const
        {
            return this->callback(this->widget, desired);
        }

        // context

        void context::update(double dt)
        {
            MX_ASSERT(_render_bounds_stack.size() == 0);
        }

        void context::push_render_bounds(const render_bounds& rb)
        {
            _render_bounds_stack.push_back(rb);
        }

        const render_bounds& context::peek_render_bounds() const
        {
            return _render_bounds_stack.back();
        }

        void context::pop_render_bounds()
        {
            _render_bounds_stack.pop_back();
        }

        void context::push_scissor(const rect& scissor)
        {
            _render_scissor_stack.push_back(scissor);
            g_nanovg.set_scissor(scissor);
        }

        void context::pop_scissor()
        {
            _render_scissor_stack.pop_back();
            g_nanovg.set_scissor(_render_scissor_stack.back());
        }

        // widget_base

        widget_base::widget_base(widget_data* data)
        : _data(data)
        {

        }

        void widget_base::focus_widget()
        {
            g_context.focus_widget(this->get_widget_id());
        }

         void widget_base::unfocus_widget()
        {
            g_context.unfocus_widget(this->get_widget_id());
        }

        bool widget_base::is_focused() const
        {
            return g_context.is_focused(this->get_widget_id());
        }

        void widget_base::resolve_style_impl(element_type_t et, element_state_t es)
        {
            this->_style = g_style_resolver.resolve(et, this->tags, es);
        }

        void widget_base::resolve_box_layout_impl(element_type_t et, widget_flag_t flag)
        {
            const auto& rb = g_context.peek_render_bounds();

            // get our box layout based off type and tags
            this->_layout = g_style_resolver.resolve_box_layout(et, this->tags);

            // override size if defined in style and not explicitly requested
            auto desiredBounds = this->_desiredBounds;
            if (!this->_desiredWidthRequested) desiredBounds.size.x = this->_layout.outerBounds.size.x;
            if (!this->_desiredHeightRequested) desiredBounds.size.y = this->_layout.outerBounds.size.y;

            // now get our actual bounds from parent
            this->_layout.outerBounds = rb.eval(desiredBounds);

            // get our style
            this->_style = g_style_resolver.resolve(et, this->tags, element_state::none);
        }

        void widget_base::resolve_scissor_impl()
        {
            const auto& rb = g_context.peek_render_bounds();
            const auto& scissor = rb.widget->childScissor();
            // calc the scissor geometry for widget specific content
            this->_scissor = get_intersecting_rect(scissor, this->_layout.outerBounds);
            this->_childScissor = get_intersecting_rect(scissor, this->_layout.inner_bounds());
        }

        void widget_base::render_box_layout_impl() const
        {
            const auto& style = this->style();

            if (style.borderSize > 0.0)
            {
                auto strokeBounds = this->_layout.border_outer_bounds();
                auto fillBounds = strokeBounds;
                fillBounds.position += vec2{style.borderSize, style.borderSize} / 2.0;
                fillBounds.size -= vec2{style.borderSize, style.borderSize};

                g_nanovg.fill_rounded_rect(fillBounds, style.borderRadius, style.backgroundColor);
                g_nanovg.stroke_rounded_rect(strokeBounds, style.borderRadius, style.borderSize, style.borderColor);
            }
            else
            {
                auto fillBounds = this->_layout.border_outer_bounds();
                g_nanovg.fill_rounded_rect(fillBounds, style.borderRadius, style.backgroundColor);
            }
        }

        void widget_base::requestSize(const vec2& size)
        {
            _desiredBounds.size = size;
            _desiredWidthRequested = true;
            _desiredHeightRequested = true;
        }

        void widget_base::requestWidth(double width)
        {
            _desiredBounds.size.x = width;
            _desiredWidthRequested = true;
        }

        void widget_base::requestHeight(double height)
        {
            _desiredBounds.size.y = height;
            _desiredHeightRequested = true;
        }

        void widget_base::requestPosition(const vec2& position)
        {
            _desiredBounds.position = position;
        }

        void widget_base::requestBounds(const rect& bounds)
        {
            _desiredBounds = bounds;
            _desiredWidthRequested = true;
            _desiredHeightRequested = true;
        }

        // root

        widget_root::widget_root(const rect& viewport)
        : widget_root(nullptr)
        {
            _viewport = viewport;
            _scissor = viewport;
            _childScissor = viewport;
            g_input.set_mouse_cursor(mouse_cursor::arrow);
            g_context.push_render_bounds({&widget_root::root_bounds_callback, this});
        }

        widget_root::~widget_root()
        {
            g_context.pop_render_bounds();
        }

        rect widget_root::bounds_callback(widget_base* parent, const rect& desired)
        {
            auto& self = *static_cast<widget_root*>(parent);
            return self._layout.child_bounds();
        }

        rect widget_root::root_bounds_callback(widget_base* parent, const rect& desired)
        {
            auto& self = *static_cast<widget_root*>(parent);
            return self._viewport;
        }

        // scroll_area

        void widget_scroll_area::resolve_scissor_impl()
        {
            const auto& rb = g_context.peek_render_bounds();
            const auto& scissor = rb.widget->scissor();

            // need to exclude the scroll area's scroll bars from scissor
            const bool scroll_vertical = this->data()->direction & scroll_direction::vertical;
            const bool scroll_horizontal = this->data()->direction & scroll_direction::horizontal;
            const auto scrollbar_size = this->style().scrollbarSize;

            // calc the scissor geometry for widget specific contetn
            // but only scissor the rendering if we're a certain type of element
            const auto padding = this->_layout.padding;
            this->_layout.padding = edges{0};
            auto childBounds = this->_layout.inner_bounds();
            this->_layout.padding = padding;
            childBounds.position += vec2{padding.left,padding.top};
            childBounds.size -= (vec2{padding.left,padding.top} + vec2{padding.right,padding.bottom});

            if (scroll_vertical)
            {
                childBounds.size.x -= scrollbar_size;
            }
            if (scroll_horizontal)
            {
                childBounds.size.y -= scrollbar_size;
            }

            this->_scissor = get_intersecting_rect(scissor, this->_layout.outerBounds);
            this->_childScissor = get_intersecting_rect(scissor, childBounds);
        }

        void widget_scroll_area::first_update_impl()
        {
            modifier_keys mods;
            vec2 pressPosition, releasePosition;
            // if (g_input.get_mouse_single_clicked(mouse_button::left, this->childScissor()))
            if (g_input.get_mouse_single_clicked(mouse_button::left, mods, pressPosition, releasePosition))
            {
                if (point_inside_rect(pressPosition, this->childScissor()) && point_inside_rect(releasePosition, this->childScissor()))
                {
                    this->focus_widget();
                }
            }
            else if (g_input.get_mouse_single_clicked(mouse_button::left))
            {
                this->unfocus_widget();
            }
        }

        rect widget_scroll_area::bounds_callback(widget_base* parent, const rect& desired)
        {
            auto& self = *static_cast<widget_scroll_area*>(parent);
            const auto& data = *self.data();
            const auto inner_bounds = self._layout.inner_bounds();
            const auto scrollbar_size = self.style().scrollbarSize;

            rect retval;
            switch(data.direction)
            {
                // stack vertically
                case scroll_direction::vertical:
                {
                    retval = inner_bounds;
                    retval.position.y += self.vertical.internal_size - data.vertical.scroll_offset;
                    retval.size.x -= scrollbar_size;
                    retval.size.y = desired.size.y;

                    self.vertical.internal_size += desired.size.y;
                    break;
                }
                // stack horizontally
                case scroll_direction::horizontal:
                {
                    retval = inner_bounds;
                    retval.position.x += self.horizontal.internal_size - data.horizontal.scroll_offset;
                    retval.size.x = desired.size.x;
                    retval.size.y -= scrollbar_size;

                    self.horizontal.internal_size += desired.size.x;
                    break;
                }
                // place wherever desired
                case scroll_direction::both:
                {
                    retval = desired;

                    self.vertical.internal_size = std::max(self.vertical.internal_size, retval.bottom());
                    self.horizontal.internal_size = std::max(self.horizontal.internal_size, retval.right());

                    retval.position += inner_bounds.position - vec2{data.horizontal.scroll_offset, data.vertical.scroll_offset};
                    break;
                }
                default:
                {
                    MX_FAIL_FAST("Unhandled direction : {}", data.direction);
                }
            }

            return retval;
        }

        void widget_scroll_area::last_update_impl()
        {
            // style constants
            const auto scrollbar_size = this->style().scrollbarSize;
            const auto scrollbar_min_length = this->style().scrollbarMinLength;
            const auto scroll_step = this->style().scrollbarStep;

            const auto bounds = this->_layout.border_inner_bounds();
            const bool scroll_vertical = this->data()->direction & scroll_direction::vertical;
            const bool scroll_horizontal = this->data()->direction & scroll_direction::horizontal;

            // valid scrollable range  (0 to scrollable_height)

            auto calc_scollabale_size = [](double internal_size, double bounds_size, double scrollbar_size) -> double
            {
                double val = internal_size - bounds_size + scrollbar_size;
                if (val < 0.0) val = 0.0;
                return val;
            };

            const auto scrollable_height = calc_scollabale_size(this->vertical.internal_size, bounds.height(), scroll_vertical ? 0.0  : scrollbar_size);
            const auto scrollable_width = calc_scollabale_size(this->horizontal.internal_size, bounds.width(), scroll_horizontal ? 0.0  : scrollbar_size);

            // size: length of the scrollbar
            // view_size: length of the viewport
            auto calc_scrollbar_grab_length = [&](double size, double view_size) -> double
            {
                if (view_size == 0.0) {
                    return size;
                }
                return clamp(scrollbar_min_length, size*size / view_size, size);
            };

            // shorthand names
            auto& vsa = this->vertical.scrollbar_area;
            auto& vsga = this->vertical.scrollbar_grab_area;

            if (scroll_vertical)
            {
                vsa =
                {
                    {bounds.right() - scrollbar_size, bounds.top()},
                    {scrollbar_size, bounds.height() - (scroll_horizontal ? scrollbar_size : 0.0)}
                };
                const double grab_length = calc_scrollbar_grab_length(vsa.height(), this->vertical.internal_size);
                vsga = (scrollable_height == 0.0) ? vsa :
                rect{
                    {vsa.left(), vsa.top() + (vsa.height() - grab_length) * (data()->vertical.scroll_offset / scrollable_height)},
                    {vsa.width(), grab_length}
                };
            }

            // shorthand names

            auto& hsa = this->horizontal.scrollbar_area;
            auto& hsga = this->horizontal.scrollbar_grab_area;

            if (scroll_horizontal)
            {
                hsa =
                {
                    {bounds.left(), bounds.bottom() - scrollbar_size},
                    {bounds.width() - (scroll_vertical ? scrollbar_size : 0.0), scrollbar_size}
                };
                const double grab_length = calc_scrollbar_grab_length(hsa.width(), this->horizontal.internal_size);
                hsga =
                {
                    {hsa.left() + (hsa.width() - grab_length) * (data()->horizontal.scroll_offset / scrollable_width), hsa.top()},
                    {grab_length, hsa.height()}
                };
            }

            /* handle input to adjust the scroll offset */

            auto prev_mouse_down = g_input.get_mouse_button_state(mouse_button::left, input_frame::previous);
            auto current_mouse_down = g_input.get_mouse_button_state(mouse_button::left, input_frame::current);
            auto current_mouse_position = g_input.get_mouse_position(input_frame::current);

            if (scroll_vertical)
            {
                if (this->vertical.internal_size < vsa.height())
                {
                    data()->vertical.scroll_offset = 0.0;
                }
                else
                {
                    // handle click and drag
                    if (!data()->vertical.grabbing)
                    {
                        // mouse clicked
                        if (!prev_mouse_down && current_mouse_down)
                        {
                            // mouse in scrollbar grab area
                            if (point_inside_rect(current_mouse_position, vsga))
                            {
                                data()->vertical.grabbing = true;
                                data()->vertical.grab_offset = current_mouse_position.y - vsga.top();
                            }
                            // mouse click on scrollbar area
                            else if(point_inside_rect(current_mouse_position, vsa))
                            {
                                data()->vertical.grabbing = true;
                                data()->vertical.grab_offset = vsga.height() / 2.0;
                                data()->vertical.scroll_offset = (current_mouse_position.y - vsa.top() - data()->vertical.grab_offset) / (vsa.height() - vsga.height()) * scrollable_height;
                            }
                        }
                    }
                    else
                    {
                        // release scrollbar
                        if(!current_mouse_down)
                        {
                            data()->vertical.grabbing = false;
                        }
                        // drag scroll
                        else
                        {
                            data()->vertical.scroll_offset = (current_mouse_position.y - vsa.top() - data()->vertical.grab_offset) / (vsa.height() - vsga.height()) * scrollable_height;
                        }
                    }
                }
            }
            if (scroll_horizontal)
            {
                if (this->horizontal.internal_size < hsa.width())
                {
                    data()->horizontal.scroll_offset = 0.0;
                }
                else
                {
                    // handle click and drag
                    if (!data()->horizontal.grabbing)
                    {
                        // mouse clicked
                        if(!prev_mouse_down && current_mouse_down)
                        {
                            // mouse in scrollbar grab area
                            if (point_inside_rect(current_mouse_position, hsga))
                            {
                                data()->horizontal.grabbing = true;
                                data()->horizontal.grab_offset = current_mouse_position.x - hsga.left();
                            }
                            // mouse click on scrollbar area
                            else if (point_inside_rect(current_mouse_position, hsa))
                            {
                                data()->horizontal.grabbing = true;
                                data()->horizontal.grab_offset = hsga.width() / 2.0;
                                data()->horizontal.scroll_offset = (current_mouse_position.x - hsa.left() - data()->horizontal.grab_offset) / (hsa.width() - hsga.width()) * scrollable_width;
                            }
                        }
                    }
                    else
                    {
                        // release scrollbar
                        if(!current_mouse_down)
                        {
                            data()->horizontal.grabbing = false;
                        }
                        // drag scroll
                        else
                        {
                            data()->horizontal.scroll_offset = (current_mouse_position.x - hsa.left() - data()->horizontal.grab_offset) / (hsa.width() - hsga.width()) * scrollable_width;
                        }
                    }
                }
            }

            if (scroll_horizontal || scroll_vertical)
            {
                // handle keyboard input
                if (this->is_focused())
                {
                    for(auto kbevent : g_input.get_keyevents())
                    {
                        if (kbevent.type == keyboard_event_type::keydown ||
                            kbevent.type == keyboard_event_type::keyrepeat)
                        {
                            const auto keycode = kbevent.keycode;
                            if (scroll_vertical)
                            {
                                switch(keycode)
                                {
                                    case GLFW_KEY_PAGE_UP:
                                        data()->vertical.scroll_offset -= 5 * scroll_step;
                                        break;
                                    case GLFW_KEY_PAGE_DOWN:
                                        data()->vertical.scroll_offset += 5 * scroll_step;
                                        break;
                                    case GLFW_KEY_UP:
                                        data()->vertical.scroll_offset -= scroll_step;
                                        break;
                                    case GLFW_KEY_DOWN:
                                        data()->vertical.scroll_offset += scroll_step;
                                        break;
                                    case GLFW_KEY_HOME:
                                        data()->vertical.scroll_offset = 0;
                                        break;
                                    case GLFW_KEY_END:
                                        data()->vertical.scroll_offset = scrollable_height;
                                        break;
                                }
                            }
                            if (scroll_horizontal)
                            {
                                switch(keycode)
                                {
                                    case GLFW_KEY_LEFT:
                                        data()->horizontal.scroll_offset -= scroll_step;
                                        break;
                                    case GLFW_KEY_RIGHT:
                                        data()->horizontal.scroll_offset += scroll_step;
                                        break;
                                }
                            }
                        }
                    }
                }

                // handle scroll wheel
                if (point_inside_rect(current_mouse_position, get_intersecting_rect(this->scissor(), this->_layout.border_inner_bounds())))
                {
                    if (!data()->vertical.grabbing)
                    {
                        data()->vertical.scroll_offset -= g_input.get_mouse_scroll(input_frame::current).y * scroll_step;
                    }
                    if (!data()->horizontal.grabbing)
                    {
                        data()->horizontal.scroll_offset -= g_input.get_mouse_scroll(input_frame::current).x * scroll_step;
                    }
                }
            }

             // clamp offsets
            data()->vertical.scroll_offset = clamp<double>(0.0, data()->vertical.scroll_offset, scrollable_height);
            data()->horizontal.scroll_offset = clamp<double>(0.0, data()->horizontal.scroll_offset, scrollable_width);
        }

        void widget_scroll_area::last_render_impl() const
        {
            const bool scroll_vertical = this->data()->direction & scroll_direction::vertical;
            const bool scroll_horizontal = this->data()->direction & scroll_direction::horizontal;
            const auto back_color = this->style().scrollbarBackColor;
            const auto fore_color = this->style().scrollbarForeColor;
            const auto active_color = this->style().scrollbarActiveColor;
            const auto corner_radius = this->style().scrollbarCornerRadius;

            const auto vsa = this->vertical.scrollbar_area;
            const auto vsga = this->vertical.scrollbar_grab_area;
            const auto hsa = this->horizontal.scrollbar_area;
            const auto hsga = this->horizontal.scrollbar_grab_area;

            if (scroll_vertical)
            {
                g_nanovg.fill_rect(vsa, back_color);

                const auto grab_color = data()->vertical.grabbing ? active_color : fore_color;
                g_nanovg.fill_rounded_rect(vsga, corner_radius, grab_color);
            }
            if (scroll_horizontal)
            {
                g_nanovg.fill_rect(hsa, back_color);

                const auto grab_color = data()->horizontal.grabbing ? active_color : fore_color;
                g_nanovg.fill_rounded_rect(hsga, corner_radius, grab_color);
            }

            if(scroll_vertical && scroll_horizontal)
            {
                const auto scrollbar_size = this->style().scrollbarSize;
                auto bottom_corner = rect{vsa.bottom_left(),vec2{scrollbar_size}};
                g_nanovg.fill_rect(bottom_corner, back_color);
            }
        }

        // Flex Box

        void widget_flex_box::resolve_box_layout_impl(element_type_t et, widget_flag_t flag)
        {
            // resolve the box layout as normal
            widget_base::resolve_box_layout_impl(et, flag);
            // calculate child sizes ahead of time
            auto& lcs = this->data()->constraints;
            if (this->direction == layout_direction::horizontal)
            {
                lcs.update_size(this->_layout.inner_bounds().width());
            }
            else if (this->direction == layout_direction::vertical)
            {
                lcs.update_size(this->_layout.inner_bounds().height());
            }

        }

        rect widget_flex_box::bounds_callback(widget_base* parent, const rect& desired)
        {
            auto& self = *static_cast<widget_flex_box*>(parent);
            auto fbd = self.data();
            const float sz = fbd->constraints.sizeAt(self._index++);

            rect retval = self._layout.child_bounds();
            if (self.direction == layout_direction::horizontal)
            {
                retval.position.x += self._offset;
                retval.size.x = sz;
            }
            else if (self.direction == layout_direction::vertical)
            {
                retval.position.y += self._offset;
                retval.size.y = sz;
            }

            self._offset += sz;

            return retval;
        }

        // Label

        void widget_label::first_render_impl() const
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
            modifier_keys mods;
            vec2 pressPosition, releasePosition;
            if (g_input.get_mouse_single_clicked(mouse_button::left, mods, pressPosition, releasePosition))
            {
                if (point_inside_rect(pressPosition, this->scissor()) && 
                    point_inside_rect(releasePosition, this->scissor()) &&
                    point_inside_rounded_rect(pressPosition, 
                                              this->_layout.border_outer_bounds(),
                                              this->_layout.borderRadius) &&
                    point_inside_rounded_rect(releasePosition, 
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

        // Text Box

        void widget_text_box::first_update_impl()
        {
            const auto mousePosition = g_input.get_mouse_position(input_frame::current);
            const auto mouseClicked = g_input.get_mouse_single_clicked(mouse_button::left);

            if(mouseClicked)
            {
                this->unfocus_widget();
            }

            if (point_inside_rect(
                mousePosition,
                this->scissor()))
            {
                if (point_inside_rounded_rect(
                    mousePosition,
                    this->_layout.border_inner_bounds(),
                    this->_layout.borderRadius - this->_layout.borderSize))
                {
                    g_input.set_mouse_cursor(mouse_cursor::ibeam);

                    if (mouseClicked)
                    {
                        this->focus_widget();
                        const auto caretPosition = g_nanovg.get_caret_position(
                                this->_layout.inner_bounds(),
                                this->data()->textOffset,
                                this->style().fontFamily,
                                this->style().fontSize,
                                this->style().textHalign,
                                this->style().textValign,
                                data()->text,
                                mousePosition);

                        auto& sb = this->data()->sb;
                        auto& selectBegin = this->data()->selectBegin;
                        auto& selectEnd = this->data()->selectEnd;

                        // select everything
                        if(g_input.get_mouse_triple_clicked(mouse_button::left))
                        {
                            selectBegin = 0;
                            selectEnd = (int32_t)(sb.end() - sb.begin());
                        }
                        // select entire word
                        else if (g_input.get_mouse_double_clicked(mouse_button::left))
                        {
                            auto calcSelectRange = [&](auto&& shouldInclude) -> void
                            {
                                // find select end (front of selection)
                                for(auto it = sb.begin() + caretPosition; it >= sb.begin(); --it)
                                {
                                    if (!shouldInclude(*it))
                                    {
                                        selectEnd = (int32_t)(it + 1 - sb.begin());
                                        break;
                                    }
                                    else if (it == sb.begin())
                                    {
                                        selectEnd = 0;
                                        break;
                                    }
                                }
                                // find select begin (back of selection)
                                for(auto it = sb.begin() + caretPosition; it < sb.end(); ++it)
                                {
                                    if (!shouldInclude(*it))
                                    {
                                        selectBegin = (int32_t)(it - sb.begin());
                                        break;
                                    }
                                    else if (it == (sb.end() - 1))
                                    {
                                        selectBegin = (int32_t)(sb.end() - sb.begin());
                                        break;
                                    }
                                }
                            };

                            // select contiguous whitespace
                            if ((sb.begin() + caretPosition)->is_whitespace())
                            {
                                calcSelectRange([](utf8_codepoint cp) -> bool {return cp.is_whitespace();});
                            }
                            // select contiguous not whitespace
                            else
                            {
                                calcSelectRange([](utf8_codepoint cp) -> bool {return !cp.is_whitespace();});
                            }
                        }
                        // put cursor under mouse
                        else
                        {
                            selectBegin = selectEnd = caretPosition;
                        }
                    }
                }
            }
        }

        void widget_text_box::last_update_impl()
        {
            if (this->is_focused())
            {
                bool textDirty = false;
                auto& sb = this->data()->sb;
                auto& selectBegin = this->data()->selectBegin;
                auto& selectEnd = this->data()->selectEnd;

                for(auto kbevent : g_input.get_keyevents())
                {
                    const bool textSelected = (selectBegin != selectEnd);
                    const auto from = std::min(selectBegin, selectEnd);
                    const auto to = std::max(selectBegin, selectEnd);

                    auto findWordBegin = [&]()
                    {
                        bool foundNonWhitespace = false;
                        auto it = sb.begin() + selectEnd;
                        while(it > sb.begin())
                        {
                            --it;
                            if (foundNonWhitespace)
                            {
                                if (it->is_whitespace())
                                {
                                    ++it;
                                    break;
                                }
                            }
                            else
                            {
                                if (!it->is_whitespace())
                                {
                                    foundNonWhitespace = true;
                                }
                            }
                        }
                        return it;
                    };
                    auto findWordEnd = [&]()
                    {
                        bool foundNonWhitespace = false;
                        auto it = sb.begin() + selectEnd;
                        while (it < sb.end())
                        {
                            if (foundNonWhitespace)
                            {
                                if (it->is_whitespace())
                                {
                                    break;
                                }
                            }
                            else
                            {
                                if (!it->is_whitespace())
                                {
                                    foundNonWhitespace = true;
                                }
                            }
                            ++it;
                        }
                        return it;
                    };

                    if (kbevent.type == keyboard_event_type::keydown ||
                        kbevent.type == keyboard_event_type::keyrepeat)
                    {
                        switch(kbevent.keycode)
                        {
                            //
                            // Navigation
                            //
                            case GLFW_KEY_LEFT:
                                // skip to begining of current word
                                if (kbevent.modifiers.control)
                                {
                                    auto it = findWordBegin();
                                    selectEnd = (it - sb.begin());
                                    if (!kbevent.modifiers.shift)
                                    {
                                        selectBegin = selectEnd;
                                    }
                                }
                                else
                                {
                                    if (textSelected)
                                    {
                                        if (kbevent.modifiers.shift)
                                        {
                                            selectEnd = std::max(selectEnd - 1, size_t(0));
                                        }
                                        else
                                        {
                                            selectBegin = std::min(selectBegin, selectEnd);
                                            selectEnd = selectBegin;
                                        }
                                    }
                                    else if (selectEnd > 0)
                                    {
                                        // move left 1
                                        selectEnd = std::max(selectEnd - 1, size_t(0));
                                        if (!kbevent.modifiers.shift)
                                        {
                                            selectBegin = selectEnd;
                                        }
                                    }
                                }
                                break;
                            case GLFW_KEY_RIGHT:
                                // skip to end of current word
                                if (kbevent.modifiers.control)
                                {
                                    auto it = findWordEnd();
                                    selectEnd = (it - sb.begin());
                                    if (!kbevent.modifiers.shift)
                                    {
                                        selectBegin = selectEnd;
                                    }
                                }
                                else
                                {
                                    if (textSelected)
                                    {
                                        if (kbevent.modifiers.shift)
                                        {
                                            selectEnd = std::min(selectEnd + 1, sb.size());
                                        }
                                        else
                                        {
                                            selectBegin = std::max(selectBegin, selectEnd);
                                            selectEnd = selectBegin;
                                        }
                                    }
                                    else if (selectEnd < sb.size())
                                    {
                                        // move right 1
                                        selectEnd = std::min(selectEnd + 1, sb.size());
                                        if (!kbevent.modifiers.shift)
                                        {
                                            selectBegin = selectEnd;
                                        }
                                    }
                                }
                                break;
                            case GLFW_KEY_UP:
                            case GLFW_KEY_HOME:
                                // begin of line
                                selectEnd = 0;
                                if (!kbevent.modifiers.shift)
                                {
                                    selectBegin = selectEnd;
                                }
                                break;
                            case GLFW_KEY_DOWN:
                            case GLFW_KEY_END:
                                // end of line
                                selectEnd = sb.size();
                                if (!kbevent.modifiers.shift)
                                {
                                    selectBegin = selectEnd;
                                }
                                break;
                            //
                            // Deletion
                            //
                            case GLFW_KEY_DELETE:
                                // delete selected text
                                if (textSelected)
                                {
                                    textDirty = true;
                                    sb.erase(sb.begin() + from, sb.begin() + to);
                                    selectBegin = selectEnd = from;
                                }
                                // delete next word
                                else if (kbevent.modifiers.control && selectEnd < sb.size())
                                {
                                    auto it = findWordEnd();
                                    sb.erase(sb.begin() + selectEnd, it);
                                    selectBegin = selectEnd;
                                    textDirty = true;
                                }
                                else if (selectEnd < sb.size())
                                {
                                    textDirty = true;
                                    sb.erase(sb.begin() + selectEnd);
                                }
                                break;
                            case GLFW_KEY_BACKSPACE:
                                // delete selected text
                                if (textSelected)
                                {
                                    textDirty = true;
                                    sb.erase(sb.begin() + from, sb.begin() + to);
                                    selectBegin = selectEnd = from;
                                }
                                // delete previous word
                                else if (kbevent.modifiers.control && selectEnd > 0)
                                {
                                    auto it = findWordBegin();
                                    sb.erase(it, sb.begin() + selectEnd);
                                    selectBegin = selectEnd = (it - sb.begin());
                                    textDirty = true;
                                }
                                // backspace from caret
                                else if (selectEnd > 0)
                                {
                                    sb.erase(sb.begin() + selectEnd - 1);
                                    selectBegin = selectEnd = selectEnd - 1;
                                    textDirty = true;
                                }
                                break;
                            //
                            // Submit
                            //
                            case GLFW_KEY_ENTER:
                            case GLFW_KEY_KP_ENTER:
                                this->on_submit(sb.str());
                                break;
                            //
                            // Keyboard Shortcuts
                            //
                            // Select All
                            case GLFW_KEY_A:
                                if (kbevent.modifiers.control)
                                {
                                    selectBegin = 0;
                                    selectEnd = (int32_t)(sb.end() - sb.begin());
                                }
                                break;
                            // Cut
                            case GLFW_KEY_X:
                                if (kbevent.modifiers.control)
                                {
                                    const auto str = sb.str(sb.begin() + from, sb.begin() + to);
                                    g_input.set_clipboard_string(str);
                                    sb.erase(sb.begin() + from, sb.begin() + to);

                                    selectEnd = selectBegin = from;
                                    textDirty = true;
                                }
                                break;
                            // Copy
                            case GLFW_KEY_C:
                                if (kbevent.modifiers.control)
                                {
                                    const auto str = sb.str(sb.begin() + from, sb.begin() + to);
                                    g_input.set_clipboard_string(str);
                                }
                                break;
                            // Paste
                            case GLFW_KEY_V:
                                if (kbevent.modifiers.control)
                                {
                                    // remove selected text
                                    sb.erase(sb.begin() + from, sb.begin() + to);
                                    // paste into sb
                                    auto initSize = sb.size();
                                    sb.insert(sb.begin() + from, g_input.get_clipboard_string());
                                    auto finalSize = sb.size();

                                    selectEnd = selectBegin = selectEnd + (int32_t)(finalSize - initSize);
                                    textDirty = true;
                                }
                                break;
                        }
                    }
                    else if(kbevent.type == keyboard_event_type::charpress)
                    {
                        textDirty = true;
                        if (textSelected)
                        {
                            sb.erase(sb.begin() + from, sb.begin() + to);
                        }

                        sb.insert(sb.begin() + selectEnd, kbevent.codepoint);
                        selectBegin = ++selectEnd;
                    }
                }

                // re-export text from buffer
                if(textDirty)
                {
                    this->data()->text = sb.str();
                }

                // update text offset                
                if (g_input.get_keyevents().size() > 0)
                {
                    const auto innerBounds = this->_layout.inner_bounds();
                    auto& textOffset = this->data()->textOffset;
                    const auto caretOffset = g_nanovg.get_caret_offset(
                            this->_layout.inner_bounds(),
                            this->style().fontFamily,
                            this->style().fontSize,
                            this->style().textHalign,
                            this->style().textValign,
                            data()->text,
                            selectEnd);  

                    if (caretOffset >= textOffset && caretOffset <= (textOffset + innerBounds.width()))
                    {
                        // do nothing text offset is fine
                    }
                    else if (caretOffset < textOffset)
                    {
                        // caret to left of view window
                        textOffset = caretOffset;
                    }
                    else
                    {
                        // caret to right of view window
                        textOffset = caretOffset - innerBounds.width();
                    }
                    textOffset = std::max(0.0, textOffset);
                }
            }
        }

        void widget_text_box::first_render_impl() const
        {
            g_context.push_scissor(this->_layout.border_inner_bounds());
            if (this->is_focused())
            {
                if (this->data()->selectBegin == this->data()->selectEnd)
                {
                    g_nanovg.draw_text_with_caret(
                        this->_layout.inner_bounds(),
                        this->data()->textOffset,
                        this->style().fontFamily,
                        this->style().fontSize,
                        this->style().fontColor,
                        this->style().textHalign,
                        this->style().textValign,
                        data()->text,
                        this->data()->selectBegin,
                        2.0f, // TODO: get from style
                        color::rgb(0,0,0));  // TODO: get from style
                }
                else
                {
                    g_nanovg.draw_text_with_selection(
                        this->_layout.inner_bounds(),
                        this->data()->textOffset,
                        this->style().fontFamily,
                        this->style().fontSize,
                        this->style().fontColor,
                        this->style().textHalign,
                        this->style().textValign,
                        data()->text,
                        std::min(this->data()->selectBegin, this->data()->selectEnd),
                        std::max(this->data()->selectBegin, this->data()->selectEnd),
                        color::rgb(0xFF,0xFF,0xFF),  // TODO: get from style
                        color::rgb(0xFF,0x88,0x00));  // TODO: get from style
                }
            }
            else
            {
                g_nanovg.draw_text(
                this->_layout.inner_bounds(),
                this->data()->textOffset,
                this->style().fontFamily,
                this->style().fontSize,
                this->style().fontColor,
                this->style().textHalign,
                this->style().textValign,
                data()->text);
            }
            g_context.pop_scissor();
        }
    }
}