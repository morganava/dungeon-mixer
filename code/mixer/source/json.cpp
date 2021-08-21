#include "serialization.hpp"
#include "paths.hpp"
#include "utilities.hpp"
#include "stylesheet.hpp"
#include "nanovg.hpp"
#include "linear_constraint_solver.hpp"
#include "logger.hpp"

using namespace dm;

namespace nlohmann
{
    // font_handle_t
    void from_json(const nl::json& j, font_handle_t& fh)
    {
        SMOL_THROW_IF_FALSE(j.is_string());

        fh = g_nanovg->create_font(paths::skins() / j.get<std::string>());
    }

    // image_handle_t
    void from_json(const nl::json& j, image_handle_t& ih)
    {
        SMOL_THROW_IF_FALSE(j.is_string());

        ih = g_nanovg->create_image(paths::skins() / j.get<std::string>());
    }

    // text alignment

    void from_json(const nl::json& j, halign_t& ha)
    {
        SMOL_THROW_IF_FALSE(j.is_string());

        const auto jstr = j.get<std::string>();
        if(jstr == "left")
        {
            ha = halign::left;
        }
        else if(jstr == "center")
        {
            ha = halign::center;
        }
        else if(jstr == "right")
        {
            ha = halign::right;
        }
        else
        {
            SMOL_THROW_MSG("Cannot parse '{}' as halign_t, must be 'left', 'center', or 'right'", jstr);
        }
    }

    void from_json(const nl::json& j, valign_t& va)
    {
        SMOL_THROW_IF_FALSE(j.is_string());

        const auto jstr = j.get<std::string>();
        if(jstr == "top")
        {
            va = valign::top;
        }
        else if(jstr == "middle")
        {
            va = valign::middle;
        }
        else if(jstr == "bottom")
        {
            va = valign::bottom;
        }
        else
        {
            SMOL_THROW_MSG("Cannot parse '{}' as valign_t, must be 'top', 'middle', or 'bottom'", jstr);
        }
    }

    // background image mode
    void from_json(const nl::json& j, background_image_mode_t& bim)
    {
        SMOL_THROW_IF_FALSE(j.is_string());

        const auto jstr = j.get<std::string>();
        constexpr const char* modeStrings[] =
        {
            "stretch",
            "tile",
            "center",
            "contain",
        };

        for(size_t i = 0; i < countof(modeStrings); ++i)
        {
            const auto* currentMode = modeStrings[i];
            if (jstr == currentMode)
            {
                bim = static_cast<background_image_mode_t>(i);
                return;
            }
        }

        SMOL_THROW_MSG("Cannot parse '{}' as background_image_mode_t, must be one of 'stretch', 'tile', 'center', or 'contain'", jstr);
    }

    // style_resolver

    void from_json(const nl::json& j, dm::element_state_t state, dm::style_declaration_set& sds)
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

    void from_json(const nl::json& js, constraint& out)
    {
        const auto parse_min = [](const nl::json& js) -> double
        {
            if (js.is_string())
            {
                auto str = js.get<std::string>();
                SMOL_THROW_IF_FALSE(str == "none");
                return 0.0;
            }
            SMOL_THROW_IF_FALSE(js.is_number());
            auto retval = js.get<double>();
            SMOL_THROW_IF_FALSE(retval >= 0.0);
            return retval;
        };

        const auto parse_max = [](const nl::json& js) -> double
        {
            if (js.is_string())
            {
                auto str = js.get<std::string>();
                SMOL_THROW_IF_FALSE(str == "none");
                return std::numeric_limits<double>::max();
            }
            SMOL_THROW_IF_FALSE(js.is_number());
            auto retval = js.get<double>();
            SMOL_THROW_IF_FALSE(retval >= 0.0);
            return retval;
        };

        const auto parse_scale = [](const nl::json& js) -> double
        {
            SMOL_THROW_IF_FALSE(js.is_number());
            auto retval = js.get<double>();
            SMOL_THROW_IF_FALSE(retval > 0.0);
            return retval;
        };

        constraint retval;

        if (js.is_array())
        {
            // purposeful fallthrough
            switch(js.size())
            {
                default:
                    static_assert(std::is_unsigned<decltype(js.size())>::value);
                case 3:
                    retval.ratio = parse_scale(js[2]);
                case 2:
                    retval.max = parse_max(js[1]);
                case 1:
                    retval.min = parse_min(js[0]);
                case 0:
                    break;
            }
        }
        else if (js.is_object())
        {
            if (auto it = js.find("ratio"); it != js.end())
            {
                retval.ratio = parse_scale(*it);
            }
            if (auto it = js.find("max"); it != js.end())
            {
                retval.max = parse_max(*it);
            }
            if (auto it = js.find("min"); it != js.end())
            {
                retval.min = parse_min(*it);
            }
        }
        else
        {
            SMOL_THROW_MSG("invalid constraint json");
        }

        MX_LOG_INFO("constraint {{ min : {}, max : {}, ratio : {} }}", retval.min, retval.max, retval.ratio);

        out = retval;
    }

    void from_json(const nl::json& js, linear_constraint_solver& out)
    {
        SMOL_THROW_IF_FALSE(js.is_array());
        auto constraints = js.get<smol::array<dm::constraint>>();
        out = {std::move(constraints)};
    }
}