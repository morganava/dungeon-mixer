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
    bool point_inside_circle(const point& p, const circle c)
    {
        vec2 line = p.position - c.position;
        double line_len_2 = line.squared_magnitude();

        return line_len_2 <= (c.radius * c.radius);
    }

    bool point_inside_rect(const point& p, const rect& r)
    {
        return (p.position.x >= r.position.x &&
            p.position.x <= (r.position.x + r.size.x) &&
            p.position.y >= r.position.y &&
            p.position.y <= (r.position.y + r.size.y));
    }
}
