#include "utilities.hpp"
#include "utf8_string.hpp"
#include "color.hpp"

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
            if (!cp.is_whitespace())
            {
                sb.push_back(cp.to_lower());
            }
        }
        str = sb.str();

        {
            float r,g,b;
            if (3 == std::sscanf(str.c_str(), "rgb(%f,%f,%f)", &r, &g, &b))
            {
                MX_THROW_IF_FALSE(r >= 0.0f && r <= 1.0f);
                MX_THROW_IF_FALSE(g >= 0.0f && g <= 1.0f);
                MX_THROW_IF_FALSE(b >= 0.0f && b <= 1.0f);
                col = color::rgb(r, g, b);
                return;
            }
        }
        {
            float r,g,b,a;
            if (4 == std::sscanf(str.c_str(), "rgba(%f,%f,%f,%f)", &r, &g, &b, &a))
            {
                MX_THROW_IF_FALSE(r >= 0.0f && r <= 1.0f);
                MX_THROW_IF_FALSE(g >= 0.0f && g <= 1.0f);
                MX_THROW_IF_FALSE(b >= 0.0f && b <= 1.0f);
                MX_THROW_IF_FALSE(a >= 0.0f && a <= 1.0f);
                col = color::rgba(r, g, b, a);
                return;
            }
        }
        MX_THROW_MSG("invalid color string: {}", str);
    }
}