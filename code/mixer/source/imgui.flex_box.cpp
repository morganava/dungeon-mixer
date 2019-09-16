#include "imgui.hpp"

namespace tabletopify
{
    namespace imgui
    {

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
    }
}