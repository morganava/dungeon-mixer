#include "color.hpp"

namespace dm
{
    namespace named_colors
    {
#define COLOR_ENTRY(NAME, R, G, B, A) dm::color NAME = {R/255.0f, G/255.0f, B/255.0f, A};
#include "color.inl"
#undef COLOR_ENTRY
    }
}