#include "geometry.hpp"

std::ostream& operator<<(std::ostream& out, const tabletopify::vec2& v)
{
    return out << "{ x : " << v.x << ", y : " << v.y << " }";
}

std::ostream& operator<<(std::ostream& out, const tabletopify::rect& r)
{
    return out << "{ position : " << r.position << " size : " << r.size << " }";
}

namespace tabletopify
{

}
