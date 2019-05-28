#include "stylesheet.hpp"
#include "utilities.hpp"

namespace tabletopify
{
    element_type_t parse_element_type(const std::string& str)
    {
        constexpr const char* typeStrings[] =
        {
            "",
            "root",
            "flexBox",
            "scrollArea",
            "spacer",
            "button",
            "label",
            "debug",
        };

        static_assert(countof(typeStrings) == (size_t)element_type::count, "mismatch of element_type enum and names");

        for(uint_fast8_t i = 0; i < countof(typeStrings); ++i)
        {
            if(str == typeStrings[i])
            {
                return static_cast<element_type_t>(i);
            }
        }
        MX_THROW_MSG("Unable to parse '{}' as element_type_t", str);
    }

    element_state_t parse_element_state(const std::string& str)
    {
        constexpr const char* stateStrings[] =
        {
            "hover",
            "active",
        };

        if (str == "")
        {
            return element_state::none;
        }

        for(uint_fast8_t i = 0; i < countof(stateStrings); ++i)
        {
            if(str == stateStrings[i])
            {
                return static_cast<element_state_t>(1 << i);
            }
        }
        MX_THROW_MSG("Unable to parse '{}' as element_state_t", str);
    }

    bool selector::operator==(const selector& that) const
    {
        return this->type == that.type &&
               this->state == that.state &&
               this->tags == that.tags;
    }

    style_sheet style_declaration_set::apply(const style_sheet& base) const
    {
        _declarations.reset();

        style_sheet retval = base;

        style_entry_t se;
        while(_declarations.next(se))
        {
            switch(se)
            {
                #define STYLE_SHEET_ENTRY(NAME, VAL, FLAGS)\
                case style_entry::NAME:\
                    {\
                        typedef std::remove_const<std::remove_reference<decltype(VAL)>::type>::type TYPE;\
                        _declarations.next<TYPE>(retval.NAME);\
                    }\
                    break;

                #include "stylesheet.inl"
                #undef STYLE_SHEET_ENTRY

                default:
                    break;
            }
        }

        return retval;
    }

    const style_sheet* style_resolver::resolve(element_type_t type, const array<utf8_string>& tags, element_state_t state) const
    {
        // build selector and return cached version if available
        selector sel = {type, tags, state};
        if(auto it = _cachedStyles.find(sel); it != _cachedStyles.end())
        {
            const auto& style = it->second;
            return &style;
        }

        element_type_t typeList[2] = {element_type::any, type};
        std::vector<element_state_t> stateList = {element_state::none};
        const auto& push_state = [&](element_state_t flag) -> void
        {
            if (static_cast<uint_fast8_t>(state) & static_cast<uint_fast8_t>(flag))
            {
                stateList.push_back(flag);
            }
        };
        push_state(element_state::hover);
        push_state(element_state::active);

        style_sheet retval;
        const auto apply_style = [&](const selector& sel) -> void
        {
            if(auto it = _styleFactory.find(sel); it != _styleFactory.end())
            {
                retval = it->second.apply(retval);
            }
        };

        for(auto currentType : typeList)
        {
            for(auto currentState : stateList)
            {
                selector s = {currentType, {}, currentState};
                apply_style(s);
            }
            for(const auto& currentTag : tags)
            {
                for(auto currentState : stateList)
                {
                    selector s = {currentType, {currentTag}, currentState};
                    apply_style(s);
                }
            }
        }

        // cache built style
        auto pit = _cachedStyles.insert(std::make_pair(std::move(sel), retval));
        return &pit.first->second;
    }

    box_layout style_resolver::resolve_box_layout(element_type_t type, const array<utf8_string>& tags) const
    {
        const auto* styleSheet = resolve(type, tags, element_state::none);
        box_layout retval;
        retval.margin = styleSheet->margin;
        retval.borderSize = styleSheet->borderSize;
        retval.borderRadius = styleSheet->borderRadius;
        retval.padding = styleSheet->padding;

        return retval;
    }
}