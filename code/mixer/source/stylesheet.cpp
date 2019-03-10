#include "stylesheet.hpp"

namespace tabletopify
{
    const style_sheet* style_resolver::resolve(const char* elementType) const
    {
        return &_rootStyle;
    }


}