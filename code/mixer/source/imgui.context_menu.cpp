#include "imgui.hpp"

namespace tabletopify
{
    namespace imgui
    {
        rect widget_context_menu::bounds_callback(widget_base* parent, const rect& desired)
        {
            return {};
        }
    }
}