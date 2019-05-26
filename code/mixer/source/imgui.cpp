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
            const auto& parent_scissor = rb.widget->_scissor;

            // get our box layout based off type and tags
            this->_layout = g_style_resolver.resolve_box_layout(et, this->tags);
            this->_layout.outerBounds = rb.eval(this->_desiredBounds);

            // get our style
            this->_style = g_style_resolver.resolve(et, this->tags, element_state::none);

            // calc the scissor geometry for widget specific contetn
            // but only scissor the rendering if we're a certain type of element
            this->_scissor = get_intersecting_rect(parent_scissor, this->_layout.outerBounds);
            if (flag & widget_flag::scissors)
            {
                g_context.push_scissor(this->_scissor);
            }
        }

        void widget_base::render_box_layout_impl() const
        {
            const auto& style = this->style();

            if (style.borderSize > 0.0)
            {
                auto strokeBounds = this->_layout.border_bounds();
                auto fillBounds = strokeBounds;
                fillBounds.position += vec2{style.borderSize, style.borderSize} / 2.0;
                fillBounds.size -= vec2{style.borderSize, style.borderSize};

                g_nanovg.fill_rounded_rect(fillBounds, style.borderRadius, style.backgroundColor);
                g_nanovg.stroke_rounded_rect(strokeBounds, style.borderRadius, style.borderSize, style.borderColor);
            }
            else
            {
                auto fillBounds = this->_layout.border_bounds();
                g_nanovg.fill_rounded_rect(fillBounds, style.borderRadius, style.backgroundColor);
            }
        }

        void widget_base::requestSize(const vec2& size)
        {
            _desiredBounds.size = size;
        }

        void widget_base::requestWidth(double width)
        {
            _desiredBounds.size.x = width;
        }

        void widget_base::requestHeight(double height)
        {
            _desiredBounds.size.y = height;
        }

        void widget_base::requestPosition(const vec2& position)
        {
            _desiredBounds.position = position;
        }

        void widget_base::requestBounds(const rect& bounds)
        {
            _desiredBounds = bounds;
        }

        // root

        widget_root::widget_root(const rect& viewport)
        : widget_root(nullptr)
        {
            _viewport = viewport;
            _scissor = viewport;
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
        void widget_scroll_area::first_update_impl()
        {
            if (g_input.get_mouse_clicked(mouse_button::left, this->_scissor))
            {
                this->focus_widget();
            }
            else if (g_input.get_mouse_clicked(mouse_button::left))
            {
                this->unfocus_widget();
            }
        }

        rect widget_scroll_area::bounds_callback(widget_base* parent, const rect& desired)
        {
            auto& self = *static_cast<widget_scroll_area*>(parent);
            const auto& data = *self.data();

            rect retval = self._layout.child_bounds();
            switch(data.direction)
            {
                case scroll_direction::vertical:
                {
                    retval.position.y += self.vertical.internal_size - data.vertical.scroll_offset;
                    retval.size.x -= self.style().scrollbarSize;
                    retval.size.y = desired.size.y;

                    self.vertical.internal_size += desired.size.y;                    
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
            const auto bounds = this->_layout.inner_bounds();
            // valid scrollable range  (0 to scrollable_height)
            const auto scrollable_height = this->vertical.internal_size - bounds.height();

            // style constants
            const auto scrollbar_size = this->style().scrollbarSize;
            const auto scrollbar_min_length = this->style().scrollbarMinLength;

            // shorthand names
            auto& vsa = this->vertical.scrollbar_area;
            auto& vsga = this->vertical.scrollbar_grab_area;

            // size: length of the scrollbar
            // view_size: length of the viewport
            auto calc_scrollbar_grab_length = [&](double size, double view_size) -> double
            {
                return clamp(scrollbar_min_length, size*size / view_size, size);
            };

            if (this->data()->direction & scroll_direction::vertical)
            {
                vsa = 
                {
                    {bounds.right() - scrollbar_size, bounds.top()},
                    {scrollbar_size, bounds.height()}
                };
                const double grab_length = calc_scrollbar_grab_length(vsa.height(), this->vertical.internal_size);
                vsga = 
                {
                    {vsa.left(), vsa.top() + (vsa.height() - grab_length) * (data()->vertical.scroll_offset / scrollable_height)},
                    {vsa.width(), grab_length}
                };    
            }
            
            /* handle input to adjust the scroll offset */

            if (this->vertical.internal_size < vsa.height())
            {
                data()->vertical.scroll_offset = 0.0;
            }
            else
            {
                auto prev_mouse_down = g_input.get_mouse_button_state(mouse_button::left, input_frame::previous);
                auto current_mouse_down = g_input.get_mouse_button_state(mouse_button::left, input_frame::current);
                auto current_mouse_position = g_input.get_mouse_position(input_frame::current);

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
                        // mouse in scrollbar area
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

                const auto scroll_step = this->style().scrollbarStep;
                // handle scroll wheel
                if (!data()->vertical.grabbing && point_inside_rect(current_mouse_position, this->_scissor))
                {
                    data()->vertical.scroll_offset -= g_input.get_mouse_scroll(input_frame::current).y * scroll_step;
                }

                // handle keyboard input
                if (this->is_focused())
                {
                    for(auto keycode : g_input.get_keypresses())
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
                }

                // clamp it
                data()->vertical.scroll_offset = clamp<double>(0, data()->vertical.scroll_offset, scrollable_height);
            }
        }
        
        void widget_scroll_area::last_render_impl() const
        {
            get_nanovg()->fill_rect(this->vertical.scrollbar_area, this->style().scrollbarBackColor);

            corners radius = this->style().scrollbarCornerRadius;

            if (data()->vertical.grabbing)
            {
                get_nanovg()->fill_rounded_rect(this->vertical.scrollbar_grab_area, radius, this->style().scrollbarActiveColor);
            }
            else
            {
                get_nanovg()->fill_rounded_rect(this->vertical.scrollbar_grab_area, radius, this->style().scrollbarForeColor);
            }
        }

        // Hbox

        void widget_hbox::resolve_box_layout_impl(element_type_t et, widget_flag_t flag)
        {
            // resolve the box layout as normal
            widget_base::resolve_box_layout_impl(et, flag);
            // calculate child sizes ahead of time
            auto& lcs = this->data()->constraints;
            lcs.update_size(this->_layout.inner_bounds().width());
            this->widths = lcs.sizes();
        }

        rect widget_hbox::bounds_callback(widget_base* parent, const rect& desired)
        {
            auto& self = *static_cast<widget_hbox*>(parent);

            float w = self.widths[self._index++];

            rect retval = self._layout.child_bounds();
            retval.position.x += self._horizontal_offset;
            retval.size.x = w;

            self._horizontal_offset += w;

            return retval;
        }

        // Label

        void widget_label::first_render_impl() const
        {
            g_nanovg.draw_text(
                this->_layout.inner_bounds(),
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
            if (point_inside_rounded_rect(
                g_input.get_mouse_position(input_frame::current),
                this->_layout.border_bounds(),
                this->_layout.borderRadius))
            {
                return element_state::hover;
            }
            return element_state::none;
        }

        void widget_button::first_update_impl()
        {
            if (g_input.get_mouse_clicked(mouse_button::left, this->_scissor))
            {
                this->focus_widget();
            }
            else if (g_input.get_mouse_clicked(mouse_button::left))
            {
                this->unfocus_widget();
            }
        }

        void widget_button::first_render_impl() const
        {
            g_nanovg.draw_text(
                this->_layout.inner_bounds(),
                this->style().fontFamily,
                this->style().fontSize,
                this->style().fontColor,
                this->style().textHalign,
                this->style().textValign,
                data()->text);
        }

        void widget_button::last_update_impl()
        {
            if (g_input.get_mouse_clicked(mouse_button::left, this->_scissor))
            {
                this->on_click();
            }
        }
    }
}