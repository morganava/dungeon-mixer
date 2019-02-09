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
        MX_ASSERT(js.is_string());
        str = (std::string)js;
    }

    // Color

    void to_json(nl::json& js, const color& col)
    {

    }
    
    void from_json(const nl::json& js, color& col)
    {
        
    }
}