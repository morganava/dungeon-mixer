#include "utf8_string.hpp"
#include "geometry.hpp"
#include "input.hpp"
#include "geometry.hpp"
#include "uuid.hpp"
#include "stylesheet.hpp"

using std::ostream;
using namespace tabletopify;

// utf8_string
ostream& operator<<(ostream& out, const utf8_string& str)
{
    return out << str.c_str();
}

ostream& operator<<(ostream& out, const utf8_string_view& sv)
{
    return out.write(sv.begin(), sv.size());
}

// mouse_button
ostream& operator<<(ostream& out, mouse_button_t btn)
{
    switch(btn)
    {
        case mouse_button::left: return out << "left";
        case mouse_button::right: return out << "right";
        case mouse_button::middle: return out << "middle";
        default: return out << "(unknown)";
    }
}

// modifier_keys
ostream& operator<<(ostream& out, modifier_keys mod)
{
    out << "[";
    const char* comma = ", ";
    const char* delimit = " ";
    if (mod.shift)
    {
        out << delimit << "shift";
        delimit = comma;
    }

    if (mod.control)
    {
        out << delimit << "control";
        delimit = comma;
    }

    if (mod.alt)
    {
        out << delimit << "alt";
        delimit = comma;
    }

    if (mod.super)
    {
        out << delimit << "super";
    }

    out << " ]";
    return out;
}

//  geometry

// ostream& operator<<(ostream& out, const vec2& v)
// {
//     return out << "{ x : " << v.x << ", y : " << v.y << " }";
// }

ostream& operator<<(ostream& out, const rect& r)
{
    return out << "{ position : " << r.position << " size : " << r.size << " }";
}

ostream& operator<<(ostream& out, const corners& c)
{
    return out << "{ topLeft : " << c.topLeft << ", topRight : " << c.topRight << ", bottomRight : " << c.bottomRight << ", bottomLeft : " << c.bottomLeft << " }";
}

ostream& operator<<(ostream& out, const edges& e)
{
    return out << "{ top : " << e.top << ", right : " << e.right << ", bottom : " << e.bottom << ", left : " << e.left << " }";
}

// SFML
ostream& operator<<(ostream& out, const sf::Vector3f& vec)
{
    return out << "{ x: " << vec.x << ", y: " << vec.y << ", z: " << vec.z << " }";
}

// uuid
ostream& operator<<(ostream& out, const struct uuid& id)
{
    const auto& bytes = id.bytes;

    fmt::fprintf(
        out, uuid::format_string,
        bytes[0], bytes[1], bytes[2], bytes[3],
        bytes[4], bytes[5], bytes[6], bytes[7],
        bytes[8], bytes[9], bytes[10], bytes[11],
        bytes[12], bytes[13], bytes[14], bytes[15]);

    return out;
}

// element_state
ostream& operator<<(ostream& out, const element_state_t& es)
{
    switch(es)
    {
        case element_state::none:
            out << "none";
            break;
        case element_state::hover:
            out << "hover";
            break;
        case element_state::active:
            out << "active";
            break;
        case element_state::disabled:
            out << "disabled";
            break;
    }
    return out;
}