#include <smol/color.hpp>
#include <smol/utf8_string.hpp>

namespace smol
{
    namespace named_colors
    {
#define COLOR_ENTRY(NAME, R, G, B, A) smol::color NAME = {R/255.0f, G/255.0f, B/255.0f, A};
#include <smol/color.inl>
#undef COLOR_ENTRY
    }
}

using namespace smol;

namespace nlohmann
{
    void to_json(nl::json& js, const color& col)
    {
        js = fmt::format("rgba({:f},{:f},{:f},{:f})", col.r, col.g, col.b, col.a);
    }

    void from_json(const nl::json& js, color& col)
    {
        SMOL_THROW_IF_FALSE(js.is_string());
        utf8_string str = (std::string)js;

        // strip out whitespace and convert it to lowercase
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
        constexpr static std::pair<const char*, const color&> args[] =
        {
#define COLOR_ENTRY(NAME, R, G, B, A) { #NAME, color::NAME },
#include <smol/color.inl>
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
            SMOL_THROW_MSG("invalid color string: {}", str);
        }
    }

    // color
    std::ostream& operator<<(std::ostream& out, const color& col)
    {
        fmt::print(out, "#{:02x}{:02x}{:02x}",
            static_cast<uint8_t>(255.0 * col.r),
            static_cast<uint8_t>(255.0 * col.g),
            static_cast<uint8_t>(255.0 * col.b));

        return out;
    }
}

