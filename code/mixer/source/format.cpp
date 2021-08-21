#include "input.hpp"
#include "stylesheet.hpp"

using std::ostream;
using namespace dm;

ostream& operator<<(ostream& out, input_event evt)
{
    return out << "{ type : " << evt.type() << " } ";
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
        delimit = comma;
    }

    if (mod.num_lock)
    {
        out << delimit << "num_lock";
    }

    out << " ]";
    return out;
}

// SFML
ostream& operator<<(ostream& out, const sf::Vector3f& vec)
{
    return out << "{ x: " << vec.x << ", y: " << vec.y << ", z: " << vec.z << " }";
}

