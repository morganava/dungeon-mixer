#include "stylesheet.hpp"
#include "utilities.hpp"

namespace tabletopify
{
    const style_sheet* style_resolver::resolve(element_type_t et, const std::vector<utf8_string>& tags, element_state_t es) const
    {
        // element_type types[] =
        // {
        //     element_type::any,
        //     et
        // };



        // start with 'any' element

        // then 'any' elements with each possible 'state'

        // then 'any' elements with each possible class

        // then 'any' elements with each possible class with each state

        // then actual element

        // then actual element with each class

        // then actual element with each class with each state

        return &_rootStyle;
    }
}