#include "stylesheet.hpp"
#include "utilities.hpp"
#include "serialization.hpp"

namespace dm
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
            "textBox",
            "slider",
            "sliderTrack",
            "sliderHandle",
            "rangeSlider",
            "rangeSliderTrack",
            "rangeSliderHandle",
            "checkbox",
            "contextMenu",
        };

        static_assert(countof(typeStrings) == static_cast<size_t>(element_type::count), "mismatch of element_type enum and names");

        for(uint_fast8_t i = 0; i < countof(typeStrings); ++i)
        {
            if(str == typeStrings[i])
            {
                return static_cast<element_type_t>(i);
            }
        }
        SMOL_THROW_MSG("Unable to parse '{}' as element_type_t", str);
    }

    element_state_t parse_element_state(const std::string& str)
    {
        constexpr const char* stateStrings[] =
        {
            "hover",
            "active",
            "disabled",
            "checked",
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
        SMOL_THROW_MSG("Unable to parse '{}' as element_state_t", str);
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
        while(_declarations.read(se))
        {
            switch(se)
            {
                #define STYLE_SHEET_ENTRY(NAME, VAL, FLAGS)\
                case style_entry::NAME:\
                    {\
                        typedef std::remove_const<std::remove_reference<decltype(VAL)>::type>::type TYPE;\
                        _declarations.read<TYPE>(retval.NAME);\
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

    style_resolver::style_resolver(fs::path&& stylesheet)
    {
        g_serialization_manager->load_stylesheet(std::move(stylesheet));
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

        // build our lists we're going to iterate through
        element_type_t typeList[] =
        {
            element_type::any,
            type
        };
        element_state_t stateList[] = {
            element_state::hover,
            element_state::active,
            element_state::disabled,
            element_state::checked,
        };

        // function to applying partial styles
        style_sheet retval;
        const auto apply_style = [&](const selector& sel) -> void
        {
            if(auto it = _styleFactory.find(sel); it != _styleFactory.end())
            {
                retval = it->second.apply(retval);
            }
        };

        // any, type
        for (auto currentType : typeList) {
            apply_style({currentType, {}, element_state::none});
        }
        // any.tag ... type.tag
        for (auto currentType : typeList) {
            for (const auto& currentTag : tags) {
                apply_style({currentType, {currentTag}, element_state::none});
            }
        }
        // any:state ... type:state
        for (auto currentType : typeList) {
            for (auto currentState : stateList) {
                if ((state & currentState) == currentState) {
                    apply_style({currentType, {}, currentState});
                }
            }
        }
        // any.tag:state ... type.tag:state
        for (auto currentType : typeList) {
            for (auto currentState : stateList) {
                if ((state & currentState) == currentState) {
                    for (const auto& currentTag : tags) {
                        apply_style({currentType, {currentTag}, currentState});
                    }
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
        retval.outerBounds.size = styleSheet->size;
        retval.margin = styleSheet->margin;
        retval.borderSize = styleSheet->borderSize;
        retval.borderRadius = styleSheet->borderRadius;
        retval.padding = styleSheet->padding;

        return retval;
    }
}

size_t std::hash<dm::selector>::operator()(const dm::selector& sel) const
{
    dm::hash<dm::element_type_t> elementTypeHash;
    dm::hash<dm::element_state_t> elementStateHash;
    dm::hash<dm::utf8_string> utf8StringHash;

    auto retval = elementTypeHash(sel.type, 0);
    for(auto& currentTag : sel.tags)
    {
        retval = utf8StringHash(currentTag, retval);
    }
    retval = elementStateHash(sel.state, retval);
    return retval;
};