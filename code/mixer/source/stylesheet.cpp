#include "stylesheet.hpp"

namespace tabletopify
{
    const style* style_resolver::resolve() const
    {
        return &_rootStyle;
    }


}