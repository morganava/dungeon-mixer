#include "utilities.hpp"
#include "utf8_string.hpp"
#include "color.hpp"
#include "uuid.hpp"
#include "stylesheet.hpp"

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
        MX_THROW_MSG("invalid color string: {}", str);
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

    // style_resolver
    void from_json(const nl::json& j, style_resolver& sr)
    {
        MX_THROW_IF_FALSE(j.is_object());

        for(auto& el : j.items())
        {
            MX_LOG_INFO("{} : {}", el.key(), el.value().dump());
            // MX_LOG_INFO("{}", el.value().dump());
            // MX_LOG_INFO("{} : {}", el.key(), el.value());
        }
    }
}