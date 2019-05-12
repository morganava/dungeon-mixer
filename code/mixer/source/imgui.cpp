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

        // widget root

        widget_root::widget_root(const rect& viewport)
        : widget_root(nullptr)
        {
            _viewport = viewport;
            g_context.push_render_bounds({&widget_root::root_bounds_callback, this, viewport});
        }

        widget_root::~widget_root()
        {
            g_context.pop_render_bounds();
        }

        rect widget_root::bounds_callback(widget_base* parent, const rect& desired)
        {
            auto& self = *static_cast<widget_root*>(parent);
            return self.layout.child_bounds();
        }

        rect widget_root::root_bounds_callback(widget_base* parent, const rect& desired)
        {
            auto& self = *static_cast<widget_root*>(parent);
            return self._viewport;
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
            g_context.focus_widget(this->get_widget_id());
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
            const auto& parent_scissor = rb.scissor;

            // get our box layout based off type and tags
            this->layout = *g_style_resolver.resolve_box_layout(et, this->tags);
            layout.outerBounds = rb.eval(this->bounds);
            this->bounds = layout.outerBounds;

            // get our style
            this->_style = g_style_resolver.resolve(et, this->tags, element_state::none);

            // calc the scissor geometry for widget specific contetn
            // but only scissor the rendering if we're a certain type of element
            this->scissor = get_intersecting_rect(parent_scissor, this->bounds);
            if (flag & widget_flag::scissors)
            {
                g_context.push_scissor(this->scissor);
            }
        }

        void widget_base::render_box_layout_impl() const
        {
            const auto& style = this->style();

            if (style.borderSize > 0.0)
            {
                auto strokeBounds = this->layout.border_bounds();
                auto fillBounds = strokeBounds;
                fillBounds.position += vec2{style.borderSize, style.borderSize} / 2.0;
                fillBounds.size -= vec2{style.borderSize, style.borderSize};

                g_nanovg.fill_rounded_rect(fillBounds, style.borderRadius, style.backgroundColor);
                g_nanovg.stroke_rounded_rect(strokeBounds, style.borderRadius, style.borderSize, style.borderColor);
            }
            else
            {
                auto fillBounds = this->layout.border_bounds();
                g_nanovg.fill_rounded_rect(fillBounds, style.borderRadius, style.backgroundColor);
            }
        }

        // vbox

        void widget_vbox::first_update_impl()
        {
            if (g_input.get_mouse_clicked(mouse_button::left, this->scissor))
            {
                this->focus_widget();
            }
            else if (g_input.get_mouse_clicked(mouse_button::left))
            {
                this->unfocus_widget();
            }
        }

        rect widget_vbox::bounds_callback(widget_base* wb, const rect& desired)
        {
            auto& self = *static_cast<widget_vbox*>(wb);
            rect retval =
            {
                {self.bounds.position.x, self.bounds.position.y + self.internal_height - self.data()->vertical_scroll},
                {self.bounds.size.x - self.style().scrollbarSize, desired.size.y}
            };
            self.internal_height += desired.size.y;
            return retval;
        }

        void widget_vbox::last_update_impl()
        {
            // height of the group
            const auto height = bounds.size.y;
            // valid scrollable range  (0 to scrollable_height)
            const auto scrollable_height = internal_height - height;

            // style constants
            const auto scrollbar_width = this->style().scrollbarSize;
            const auto min_scrollbar_height = this->style().scrollbarMinLength;

            // scrollbar bg rect
            scrollbar_area = {{bounds.position.x + bounds.size.x - scrollbar_width, bounds.position.y}, {scrollbar_width, height}};

            // scrollbar rect
            double scrollbar_x = bounds.position.x  + bounds.size.x - scrollbar_width;

            double scrollbar_height = std::min(std::max(height*height / internal_height, min_scrollbar_height), height);
            double scrollbar_y = bounds.position.y + (height - scrollbar_height) * (data()->vertical_scroll / scrollable_height);

            scrollbar_grab_area = {{scrollbar_x, scrollbar_y}, {scrollbar_width, scrollbar_height}};

            if (internal_height < height)
            {
                data()->vertical_scroll = 0.0;
            }
            else
            {
                auto prev_mouse_down = g_input.get_mouse_button_state(mouse_button::left, input_frame::previous);
                auto current_mouse_down = g_input.get_mouse_button_state(mouse_button::left, input_frame::current);
                auto current_mouse_position = g_input.get_mouse_position(input_frame::current);

                // handle click and drag
                if (!data()->grabbing_scrollbar)
                {
                    // mouse clicked
                    if (!prev_mouse_down && current_mouse_down)
                    {
                        // mouse in scrollbar grab area
                        if (point_inside_rect(current_mouse_position, scrollbar_grab_area))
                        {
                            data()->grabbing_scrollbar = true;
                            data()->grab_offset_y = current_mouse_position.y - scrollbar_grab_area.position.y;
                        }
                        // mouse in scrollbar area
                        else if(point_inside_rect(current_mouse_position, scrollbar_area))
                        {
                            data()->grabbing_scrollbar = true;
                            data()->grab_offset_y = scrollbar_grab_area.size.y / 2.0;
                            data()->vertical_scroll = (current_mouse_position.y - bounds.position.y - data()->grab_offset_y) /
                                                    (bounds.size.y - scrollbar_grab_area.size.y) * scrollable_height;
                        }
                    }
                }
                else
                {
                    // release scrollbar
                    if(!current_mouse_down)
                    {
                        data()->grabbing_scrollbar = false;
                    }
                    // drag scroll
                    else
                    {
                        data()->vertical_scroll = (current_mouse_position.y - bounds.position.y - data()->grab_offset_y) /
                                                (bounds.size.y - scrollbar_grab_area.size.y) * scrollable_height;
                    }
                }

                const auto scroll_step = this->style().scrollbarStep;
                // handle scroll wheel
                if (!data()->grabbing_scrollbar && point_inside_rect(current_mouse_position, this->scissor))
                {
                    data()->vertical_scroll -= g_input.get_mouse_scroll(input_frame::current).y * scroll_step;
                }

                // handle keyboard input
                if (this->is_focused())
                {
                    for(auto keycode : g_input.get_keypresses())
                    {
                        switch(keycode)
                        {
                            case GLFW_KEY_PAGE_UP:
                                data()->vertical_scroll -= 5 * scroll_step;
                                break;
                            case GLFW_KEY_PAGE_DOWN:
                                data()->vertical_scroll += 5 * scroll_step;
                                break;
                            case GLFW_KEY_UP:
                                data()->vertical_scroll -= scroll_step;
                                break;
                            case GLFW_KEY_DOWN:
                                data()->vertical_scroll += scroll_step;
                                break;
                            case GLFW_KEY_HOME:
                                data()->vertical_scroll = 0;
                                break;
                            case GLFW_KEY_END:
                                data()->vertical_scroll = scrollable_height;
                                break;
                        }
                    }
                }

                // clamp it
                data()->vertical_scroll = clamp<double>(0, data()->vertical_scroll, scrollable_height);
            }
        }

        void widget_vbox::last_render_impl() const
        {
            get_nanovg()->fill_rect(scrollbar_area, this->style().scrollbarBackColor);

            corners radius = this->style().scrollbarCornerRadius;

            if (data()->grabbing_scrollbar)
            {
                get_nanovg()->fill_rounded_rect(scrollbar_grab_area, radius, this->style().scrollbarActiveColor);
            }
            else
            {
                get_nanovg()->fill_rounded_rect(scrollbar_grab_area, radius, this->style().scrollbarForeColor);
            }

        }

        // Hbox

        rect widget_hbox::bounds_callback(widget_base* parent, const rect& desired)
        {
            auto& self = *static_cast<widget_hbox*>(parent);

            float w = self.widths[self._index++];

            rect retval =
            {
                {self.bounds.position.x + self._horizontal_offset, self.bounds.position.y},
                {w, self.bounds.size.y}
            };

            self._horizontal_offset += w;

            return retval;
        }

        // Debug

        void widget_debug::first_update_impl()
        {
            auto current_position = g_input.get_mouse_position(input_frame::current);
            auto previous_position = g_input.get_mouse_position(input_frame::previous);
            if (point_inside_rect(current_position, scissor))
            {
                on_mouse_over(current_position);

                if (current_position != previous_position)
                {
                    on_mouse_move(current_position);
                }
            }
        }
        void widget_debug::first_render_impl() const
        {
            get_nanovg()->fill_rect(bounds, color::rgb(0,0,0));
            get_nanovg()->fill_rect(rect{bounds.position + vec2{1.0, 1.0}, {bounds.size - vec2{2.0, 2.0}}}, bg_color);
            // get_nanovg()->fill_rounded_rect((rectf)rect{bounds.position + vec2{1.0, 1.0}, {bounds.size - vec2{2.0, 2.0}}}, cornersf{10.0f}, color::rgb(0x77, 0x77, 0x77));
        }

        // Label

        void widget_label::first_render_impl() const
        {
            g_nanovg.draw_text(
                bounds,
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
                layout.border_bounds(),
                layout.borderRadius))
            {
                return element_state::hover;
            }
            return element_state::none;
        }

        void widget_button::first_update_impl()
        {
            if (g_input.get_mouse_clicked(mouse_button::left, this->scissor))
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
                bounds,
                this->style().fontFamily,
                this->style().fontSize,
                this->style().fontColor,
                this->style().textHalign,
                this->style().textValign,
                data()->text);
        }

        void widget_button::last_update_impl()
        {
            if (g_input.get_mouse_clicked(mouse_button::left, this->scissor))
            {
                this->on_click();
            }
        }
    }
}