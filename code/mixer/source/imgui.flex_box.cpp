#include "imgui.hpp"

namespace tabletopify
{
    namespace imgui
    {
        // Flex Box
        void widget_flex_box::resolve_box_layout_impl(element_type_t et, widget_flag_t flags)
        {
            // resolve the box layout as normal
            widget_base::resolve_box_layout_impl(et, flags);
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

        rect widget_flex_box::bounds_callback_impl(rect desired)
        {
            auto& self = *this;
            auto* fbd = self.data();
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

            if (desired.height() > 0.0 && desired.height() < retval.height()) {
                retval.position.y += (retval.height() - desired.height()) / 2.0;
                retval.size.y = desired.height();
            }

            if (desired.width() > 0.0 && desired.width() < retval.width()) {
                retval.position.x += (retval.width() - desired.width()) / 2.0;
                retval.size.x = desired.width();
            }

            return retval;
        }
    }
}