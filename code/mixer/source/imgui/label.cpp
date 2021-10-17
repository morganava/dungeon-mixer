#include "../imgui.hpp"

namespace dm
{
    namespace imgui
    {
        // Label

        void widget_label::first_render_impl() const
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
    }
}