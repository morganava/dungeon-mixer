#include "logger.hpp"
#include "stylesheet.hpp"
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
} /* dm */

using namespace dm;
namespace nlohmann
{
    void from_json(const nl::json& j, element_state_t state, style_declaration_set& sds)
    {
        SMOL_THROW_IF_FALSE(j.is_object());

        auto& d = sds._declarations;

        #define STYLE_SHEET_ENTRY(NAME, VAL, FLAGS)\
        if(auto it = j.find(#NAME); it != j.end())\
        {\
            if constexpr(FLAGS != element_state::none) SMOL_THROW_MSG_IF_FALSE(!static_cast<uint_fast8_t>(FLAGS & state), "style property '{}' cannot be specified in styles with '{}' selector", #NAME, state);\
            d.write<style_entry_t>(style_entry::NAME);\
            typedef std::remove_const<std::remove_reference<decltype(VAL)>::type>::type TYPE;\
            d.write<TYPE>(it->get<TYPE>());\
        }\

        #include "stylesheet.inl"
        #undef STYLE_SHEET_ENTRY
    }

    void from_json(const nl::json& j, style_resolver& sr)
    {
        SMOL_THROW_IF_FALSE(j.is_object());

        // iterate over each selector and build our style selector set
        const std::regex selectorRegex("^([a-zA-Z]*)[\\.]?([a-zA-Z0-9]*)[\\:]?([a-z]*)$");
        for(auto& el : j.items())
        {
            const auto& selectorString = el.key();
            std::smatch matches;
            if(std::regex_match(selectorString, matches, selectorRegex))
            {
                MX_LOG_INFO("'{}' selector matches", selectorString);

                const auto& elementStr = matches[1].str();
                const auto& tagStr = matches[2].str();
                const auto& stateStr = matches[3].str();

                MX_LOG_INFO(" element : '{}'", elementStr);
                MX_LOG_INFO(" tag : '{}'", tagStr);
                MX_LOG_INFO(" state : '{}'", stateStr);

                MX_LOG_INFO(" rule : {}", el.value().dump());

                auto element = parse_element_type(elementStr);
                auto state = parse_element_state(stateStr);

                style_declaration_set sds;
                nl::from_json(el.value(), state, sds);

                MX_LOG_INFO(" element : {}, state : {}", (uint_fast8_t)element, (uint_fast8_t)state);

                selector sel = (tagStr == "") ? selector{element, {}, state} : selector{element, array<utf8_string>{tagStr}, state};
                sr._styleFactory.emplace(std::make_pair(std::move(sel), sds));
            }
        }
    }
} /* nlohmann */

size_t std::hash<dm::selector>::operator()(const dm::selector& sel) const
{
    size_t retval = smol::calc_hash(sel.type);
    retval = smol::calc_hash(sel.tags, retval);
    retval = smol::calc_hash(sel.state, retval);

    return retval;
};