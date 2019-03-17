#include "stylesheet.hpp"
#include "utilities.hpp"

namespace tabletopify
{


    style_sheet style_declaration_set::apply(const style_sheet& base) const
    {
        _declarations.reset();

        style_sheet retval = base;

        style_entry_t se;
        while(_declarations.next(se))
        {
            switch(se)
            {
                #define STYLE_SHEET_ENTRY(TYPE, NAME, VAL)\
                case style_entry::NAME:\
                    _declarations.next<TYPE>(retval.NAME);\
                    break;

                #include "stylesheet.inl"
                #undef STYLE_SHEET_ENTRY

                default:
                    break;
            }
        }

        return retval;
    }


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