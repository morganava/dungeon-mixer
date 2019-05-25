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
        utf8_stringbuffer sb(str.size() + 1);
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

        constexpr static std::pair<const char*, color> args[] =
        {
            {"aliceblue", color::rgb(240,248,255)},
            {"antiquewhite", color::rgb(250,235,215)},
            {"aqua", color::rgb(0,255,255)},
            {"aquamarine", color::rgb(127,255,212)},
            {"azure", color::rgb(240,255,255)},
            {"beige", color::rgb(245,245,220)},
            {"bisque", color::rgb(255,228,196)},
            {"black", color::rgb(0,0,0)},
            {"blanchedalmond", color::rgb(255,235,205)},
            {"blue", color::rgb(0,0,255)},
            {"blueviolet", color::rgb(138,43,226)},
            {"brown", color::rgb(165,42,42)},
            {"burlywood", color::rgb(222,184,135)},
            {"cadetblue", color::rgb(95,158,160)},
            {"chartreuse", color::rgb(127,255,0)},
            {"chocolate", color::rgb(210,105,30)},
            {"coral", color::rgb(255,127,80)},
            {"cornflowerblue", color::rgb(100,149,237)},
            {"cornsilk", color::rgb(255,248,220)},
            {"crimson", color::rgb(220,20,60)},
            {"cyan", color::rgb(0,255,255)},
            {"darkblue", color::rgb(0,0,139)},
            {"darkcyan", color::rgb(0,139,139)},
            {"darkgoldenrod", color::rgb(184,134,11)},
            {"darkgray", color::rgb(169,169,169)},
            {"darkgreen", color::rgb(0,100,0)},
            {"darkgrey", color::rgb(169,169,169)},
            {"darkkhaki", color::rgb(189,183,107)},
            {"darkmagenta", color::rgb(139,0,139)},
            {"darkolivegreen", color::rgb(85,107,47)},
            {"darkorange", color::rgb(255,140,0)},
            {"darkorchid", color::rgb(153,50,204)},
            {"darkred", color::rgb(139,0,0)},
            {"darksalmon", color::rgb(233,150,122)},
            {"darkseagreen", color::rgb(143,188,143)},
            {"darkslateblue", color::rgb(72,61,139)},
            {"darkslategray", color::rgb(47,79,79)},
            {"darkslategrey", color::rgb(47,79,79)},
            {"darkturquoise", color::rgb(0,206,209)},
            {"darkviolet", color::rgb(148,0,211)},
            {"deeppink", color::rgb(255,20,147)},
            {"deepskyblue", color::rgb(0,191,255)},
            {"dimgray", color::rgb(105,105,105)},
            {"dimgrey", color::rgb(105,105,105)},
            {"dodgerblue", color::rgb(30,144,255)},
            {"firebrick", color::rgb(178,34,34)},
            {"floralwhite", color::rgb(255,250,240)},
            {"forestgreen", color::rgb(34,139,34)},
            {"fuchsia", color::rgb(255,0,255)},
            {"gainsboro", color::rgb(220,220,220)},
            {"ghostwhite", color::rgb(248,248,255)},
            {"gold", color::rgb(255,215,0)},
            {"goldenrod", color::rgb(218,165,32)},
            {"gray", color::rgb(128,128,128)},
            {"green", color::rgb(0,128,0)},
            {"greenyellow", color::rgb(173,255,47)},
            {"grey", color::rgb(128,128,128)},
            {"honeydew", color::rgb(240,255,240)},
            {"hotpink", color::rgb(255,105,180)},
            {"indianred", color::rgb(205,92,92)},
            {"indigo", color::rgb(75,0,130)},
            {"ivory", color::rgb(255,255,240)},
            {"khaki", color::rgb(240,230,140)},
            {"lavender", color::rgb(230,230,250)},
            {"lavenderblush", color::rgb(255,240,245)},
            {"lawngreen", color::rgb(124,252,0)},
            {"lemonchiffon", color::rgb(255,250,205)},
            {"lightblue", color::rgb(173,216,230)},
            {"lightcoral", color::rgb(240,128,128)},
            {"lightcyan", color::rgb(224,255,255)},
            {"lightgoldenrodyellow", color::rgb(250,250,210)},
            {"lightgray", color::rgb(211,211,211)},
            {"lightgreen", color::rgb(144,238,144)},
            {"lightgrey", color::rgb(211,211,211)},
            {"lightpink", color::rgb(255,182,193)},
            {"lightsalmon", color::rgb(255,160,122)},
            {"lightseagreen", color::rgb(32,178,170)},
            {"lightskyblue", color::rgb(135,206,250)},
            {"lightslategray", color::rgb(119,136,153)},
            {"lightslategrey", color::rgb(119,136,153)},
            {"lightsteelblue", color::rgb(176,196,222)},
            {"lightyellow", color::rgb(255,255,224)},
            {"lime", color::rgb(0,255,0)},
            {"limegreen", color::rgb(50,205,50)},
            {"linen", color::rgb(250,240,230)},
            {"magenta", color::rgb(255,0,255)},
            {"maroon", color::rgb(128,0,0)},
            {"mediumaquamarine", color::rgb(102,205,170)},
            {"mediumblue", color::rgb(0,0,205)},
            {"mediumorchid", color::rgb(186,85,211)},
            {"mediumpurple", color::rgb(147,112,219)},
            {"mediumseagreen", color::rgb(60,179,113)},
            {"mediumslateblue", color::rgb(123,104,238)},
            {"mediumspringgreen", color::rgb(0,250,154)},
            {"mediumturquoise", color::rgb(72,209,204)},
            {"mediumvioletred", color::rgb(199,21,133)},
            {"midnightblue", color::rgb(25,25,112)},
            {"mintcream", color::rgb(245,255,250)},
            {"mistyrose", color::rgb(255,228,225)},
            {"moccasin", color::rgb(255,228,181)},
            {"navajowhite", color::rgb(255,222,173)},
            {"navy", color::rgb(0,0,128)},
            {"oldlace", color::rgb(253,245,230)},
            {"olive", color::rgb(128,128,0)},
            {"olivedrab", color::rgb(107,142,35)},
            {"orange", color::rgb(255,165,0)},
            {"orangered", color::rgb(255,69,0)},
            {"orchid", color::rgb(218,112,214)},
            {"palegoldenrod", color::rgb(238,232,170)},
            {"palegreen", color::rgb(152,251,152)},
            {"paleturquoise", color::rgb(175,238,238)},
            {"palevioletred", color::rgb(219,112,147)},
            {"papayawhip", color::rgb(255,239,213)},
            {"peachpuff", color::rgb(255,218,185)},
            {"peru", color::rgb(205,133,63)},
            {"pink", color::rgb(255,192,203)},
            {"plum", color::rgb(221,160,221)},
            {"powderblue", color::rgb(176,224,230)},
            {"purple", color::rgb(128,0,128)},
            {"rebeccapurple", color::rgb(102,51,153)},
            {"red", color::rgb(255,0,0)},
            {"rosybrown", color::rgb(188,143,143)},
            {"royalblue", color::rgb(65,105,225)},
            {"saddlebrown", color::rgb(139,69,19)},
            {"salmon", color::rgb(250,128,114)},
            {"sandybrown", color::rgb(244,164,96)},
            {"seagreen", color::rgb(46,139,87)},
            {"seashell", color::rgb(255,245,238)},
            {"sienna", color::rgb(160,82,45)},
            {"silver", color::rgb(192,192,192)},
            {"skyblue", color::rgb(135,206,235)},
            {"slateblue", color::rgb(106,90,205)},
            {"slategray", color::rgb(112,128,144)},
            {"slategrey", color::rgb(112,128,144)},
            {"snow", color::rgb(255,250,250)},
            {"springgreen", color::rgb(0,255,127)},
            {"steelblue", color::rgb(70,130,180)},
            {"tan", color::rgb(210,180,140)},
            {"teal", color::rgb(0,128,128)},
            {"thistle", color::rgb(216,191,216)},
            {"tomato", color::rgb(255,99,71)},
            {"transparent", color::rgba(0,0,0,0.0)},
            {"turquoise", color::rgb(64,224,208)},
            {"violet", color::rgb(238,130,238)},
            {"wheat", color::rgb(245,222,179)},
            {"white", color::rgb(255,255,255)},
            {"whitesmoke", color::rgb(245,245,245)},
            {"yellow", color::rgb(255,255,0)},
            {"yellowgreen", color::rgb(154,205,50)},
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

    // font_id
    void from_json(const nl::json& j, font_id& fi)
    {
        MX_THROW_IF_FALSE(j.is_string());

        fi = g_nanovg.create_font(get_skins_path() / j.get<std::string>());
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

    // style_resolver

    void from_json(const nl::json& j, element_state_t state, style_declaration_set& sds)
    {
        MX_THROW_IF_FALSE(j.is_object());

        auto& d = sds._declarations;

        #define STYLE_SHEET_ENTRY(NAME, VAL, FLAGS)\
        if(auto it = j.find(#NAME); it != j.end())\
        {\
            if constexpr(FLAGS & sdf::geometry) MX_THROW_MSG_IF_FALSE(state == element_state::none, "style property '{}' cannot be specified in styles with '{}' selector", #NAME, state);\
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
        const std::regex selectorRegex("^([a-z]*)[\\.]?([a-z]*)[\\:]?([a-z]*)$");
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

                MX_LOG_INFO(" element : {}", elementStr);
                MX_LOG_INFO(" tag : {}", tagStr);
                MX_LOG_INFO(" state : {}", stateStr);

                MX_LOG_INFO(" rule : {}", el.value().dump());

                auto element = parse_element_type(elementStr);
                auto state = parse_element_state(stateStr);

                style_declaration_set sds;
                from_json(el.value(), state, sds);

                MX_LOG_INFO(" element : {}, state : {}", (uint_fast8_t)element, (uint_fast8_t)state);

                sr._styleFactory.insert({{element, tagStr, state}, sds});
            }
        }
    }
}