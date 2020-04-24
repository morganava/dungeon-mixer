#include "serialization.hpp"
#include "utilities.hpp"
#include "utf8_string.hpp"
#include "color.hpp"
#include "uuid.hpp"
#include "geometry.hpp"
#include "stylesheet.hpp"
#include "nanovg.hpp"

namespace tabletopify
{
    // UTF8 String

    void to_json(nl::json& js, const utf8_string& str)
    {
        js = str.str();
    }

    void from_json(const nl::json& js, utf8_string& str)
    {
        MX_THROW_IF_FALSE(js.is_string());
        str = (std::string)js;
    }

    // Color

    void to_json(nl::json& js, const color& col)
    {
        js = fmt::format("rgba({:f},{:f},{:f},{:f})", col.r, col.g, col.b, col.a);
    }

    void from_json(const nl::json& js, color& col)
    {
        MX_THROW_IF_FALSE(js.is_string());
        utf8_string str = (std::string)js;

        // strip out whitespace and conver it to lowercase
        utf8_stringbuffer sb;
        sb.reserve(str.size());

        for(auto cp : str)
        {
            if(!cp.is_whitespace())
            {
                sb.push_back(cp.to_lower());
            }
        }
        str = sb.str();

        // rgb(0,0,0)
        {
            uint32_t r,g,b;
            if(3 == std::sscanf(str.c_str(), "rgb(%u,%u,%u)", &r, &g, &b))
            {
                col = color::rgb(r, g, b);
                return;
            }
        }
        //  rgb(0,0,0,1.0)
        {
            uint32_t r,g,b;
            float a;
            if(4 == std::sscanf(str.c_str(), "rgba(%u,%u,%u,%f)", &r, &g, &b, &a))
            {
                col = color::rgba(r, g, b, a);
                return;
            }
        }
        // #000000
        {
            uint32_t r,g,b;
            if(3 == std::sscanf(str.c_str(), "#%02x%02x%02x", &r, &g, &b))
            {
                col = color::rgb(r,g,b);
                return;
            }
        }
        // #000
        {
            uint32_t r,g,b;
            if(3 == std::sscanf(str.c_str(), "#%01x%01x%01x", &r, &g, &b))
            {
                r *= 17;
                g *= 17;
                b *= 17;

                col = color::rgb(r,g,b);
                return;
            }
        }

        // sorted for binary sort
        constexpr static std::pair<const char*, color> args[] =
        {
#define COLOR_ENTRY(NAME, RGB) { #NAME, color::RGB },
#include "color.inl"
#undef COLOR_ENTRY
        };

        auto it = std::lower_bound(begin(args), end(args), str.c_str(), [](const auto& left, const char* right) -> bool
        {
            return strcmp(left.first, right) < 0;
        });

        if (it != end(args) &&
            !strcmp(it->first, str.c_str()))
        {
            col = it->second;
        }
        else
        {
            MX_THROW_MSG("invalid color string: {}", str);
        }
    }

    // UUID
    void to_json(nl::json& j, const struct uuid& id)
    {
        const auto& bytes = id.bytes;
        j = fmt::sprintf(
            uuid::format_string,
            bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7],
            bytes[8], bytes[9], bytes[10], bytes[11],
            bytes[12], bytes[13], bytes[14], bytes[15]);
    }

    void from_json(const nl::json& j, struct uuid& id)
    {
        MX_THROW_IF_FALSE(j.is_string());

        auto& bytes = id.bytes;
        std::string uuidString = j;

        MX_THROW_IF_FALSE(uuidString.size() == uuid::string_length);
        MX_THROW_IF_FALSE(16 == sscanf(uuidString.data(),
            uuid::format_string,
            &bytes[0], &bytes[1], &bytes[2], &bytes[3],
            &bytes[4], &bytes[5], &bytes[6], &bytes[7],
            &bytes[8], &bytes[9], &bytes[10], &bytes[11],
            &bytes[12], &bytes[13], &bytes[14], &bytes[15]));
    }

    // Geometry

    void from_json(const nl::json& j, vec2& v)
    {
        if (j.is_object())
        {
            if(auto it = j.find("x"); it != j.end())
            {
                MX_THROW_IF_FALSE(it->is_number());
                v.x = it->get<double>();
            }
            if(auto it = j.find("y"); it != j.end())
            {
                MX_THROW_IF_FALSE(it->is_number());
                v.y = it->get<double>();
            }
        }
        else if(j.is_array())
        {
            MX_THROW_IF_FALSE(j.size() == 2);
            MX_THROW_IF_FALSE(j[0].is_number());
            MX_THROW_IF_FALSE(j[1].is_number());
            v.x = j[0].get<double>();
            v.y = j[1].get<double>();
        }
    }

    void from_json(const nl::json& j, corners& c)
    {
        if (j.is_object())
        {
            if(auto it = j.find("topLeft"); it != j.end())
            {
                MX_THROW_IF_FALSE(it->is_number());
                c.topLeft = it->get<double>();
            }
            if(auto it = j.find("topRight"); it != j.end())
            {
                MX_THROW_IF_FALSE(it->is_number());
                c.topRight = it->get<double>();
            }
            if(auto it = j.find("bottomRight"); it != j.end())
            {
                MX_THROW_IF_FALSE(it->is_number());
                c.bottomRight = it->get<double>();
            }
            if(auto it = j.find("bottomLeft"); it != j.end())
            {
                MX_THROW_IF_FALSE(it->is_number());
                c.bottomLeft = it->get<double>();
            }
        }
        else if(j.is_array())
        {
            MX_THROW_IF_FALSE(j.size() == 4);
            for(size_t i = 0; i < 4; ++i)
            {
                const auto& j_i = j[i];
                MX_THROW_IF_FALSE(j_i.is_number());
                c.raw[i] = j_i.get<double>();
            }
        }
        else if(j.is_number())
        {
            const auto val = j.get<double>();
            for(size_t i = 0; i < 4; ++i)
            {
                c.raw[i] = val;
            }
        }
    }

    void from_json(const nl::json& j, edges& e)
    {
        if (j.is_object())
        {
            if(auto it = j.find("top"); it != j.end())
            {
                MX_THROW_IF_FALSE(it->is_number());
                e.top = it->get<double>();
            }
            if(auto it = j.find("right"); it != j.end())
            {
                MX_THROW_IF_FALSE(it->is_number());
                e.right = it->get<double>();
            }
            if(auto it = j.find("bottom"); it != j.end())
            {
                MX_THROW_IF_FALSE(it->is_number());
                e.bottom = it->get<double>();
            }
            if(auto it = j.find("left"); it != j.end())
            {
                MX_THROW_IF_FALSE(it->is_number());
                e.left = it->get<double>();
            }
        }
        else if(j.is_array())
        {
            MX_THROW_IF_FALSE(j.size() == 4);
            for(size_t i = 0; i < 4; ++i)
            {
                const auto& j_i = j[i];
                MX_THROW_IF_FALSE(j_i.is_number());
                e.raw[i] = j_i.get<double>();
            }
        }
        else if(j.is_number())
        {
            const auto val = j.get<double>();
            for(size_t i = 0; i < 4; ++i)
            {
                e.raw[i] = val;
            }
        }
    }

    // font_handle_t
    void from_json(const nl::json& j, font_handle_t& fh)
    {
        MX_THROW_IF_FALSE(j.is_string());

        fh = g_nanovg.create_font(get_skins_path() / j.get<std::string>());
    }

    // image_handle_t
    void from_json(const nl::json& j, image_handle_t& ih)
    {
        MX_THROW_IF_FALSE(j.is_string());

        ih = g_nanovg.create_image(get_skins_path() / j.get<std::string>());
    }

    // text alignment

    void from_json(const nl::json& j, halign_t& ha)
    {
        MX_THROW_IF_FALSE(j.is_string());

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
            MX_THROW_MSG("Cannot parse '{}' as halign_t, must be 'left', 'center', or 'right'", jstr);
        }
    }

    void from_json(const nl::json& j, valign_t& va)
    {
        MX_THROW_IF_FALSE(j.is_string());

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
            MX_THROW_MSG("Cannot parse '{}' as valign_t, must be 'top', 'middle', or 'bottom'", jstr);
        }
    }

    // background image mode
    void from_json(const nl::json& j, background_image_mode_t& bim)
    {
        MX_THROW_IF_FALSE(j.is_string());

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

        MX_THROW_MSG("Cannot parse '{}' as background_image_mode_t, must be one of 'stretch', 'tile', 'center', or 'contain'", jstr);
    }

    // style_resolver

    void from_json(const nl::json& j, element_state_t state, style_declaration_set& sds)
    {
        MX_THROW_IF_FALSE(j.is_object());

        auto& d = sds._declarations;

        #define STYLE_SHEET_ENTRY(NAME, VAL, FLAGS)\
        if(auto it = j.find(#NAME); it != j.end())\
        {\
            if constexpr(FLAGS != element_state::none) MX_THROW_MSG_IF_FALSE(!static_cast<uint_fast8_t>(FLAGS & state), "style property '{}' cannot be specified in styles with '{}' selector", #NAME, state);\
            d.push_back<style_entry_t>(style_entry::NAME);\
            typedef std::remove_const<std::remove_reference<decltype(VAL)>::type>::type TYPE;\
            d.push_back<TYPE>(it->get<TYPE>());\
        }\

        #include "stylesheet.inl"
        #undef STYLE_SHEET_ENTRY
    }

    void from_json(const nl::json& j, style_resolver& sr)
    {
        MX_THROW_IF_FALSE(j.is_object());

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
                from_json(el.value(), state, sds);

                MX_LOG_INFO(" element : {}, state : {}", (uint_fast8_t)element, (uint_fast8_t)state);

                selector sel = (tagStr == "") ? selector{element, {}, state} : selector{element, array<utf8_string>{tagStr}, state};
                sr._styleFactory.emplace(std::make_pair(std::move(sel), sds));
            }
        }
    }
}