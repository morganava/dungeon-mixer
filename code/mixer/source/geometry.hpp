#pragma once

namespace tabletopify
{
    struct vec2
    {
        double x = 0.0;
        double y = 0.0;

        constexpr vec2() = default;
        constexpr vec2(const vec2&) = default;
        constexpr vec2(vec2&&) = default;
        
        constexpr vec2(double x, double y)
        : x(x)
        , y(y)
        { }
        
        constexpr vec2& operator=(const vec2&) = default;

        constexpr vec2 operator+(const vec2& that) const
        {
            return vec2(this->x + that.x, this->y + that.y);
        }

        constexpr vec2 operator-(const vec2& that) const
        {
            return vec2(this->x - that.x, this->y - that.y);
        }

        constexpr vec2 operator*(double d) const
        {
            return vec2(this->x * d, this->y * d);
        }

        constexpr static double dot_product(const vec2& a, const vec2& b)
        {
            return a.x*b.x + a.y*b.y;
        }

        constexpr double squared_magnitude() const
        {
            return x*x + y*y;
        }
    };

    struct point
    {
        vec2 position;
    };

    struct rect
    {
        vec2 position;
        vec2 size;
    };

    struct circle
    {
        vec2 position;
        double radius;
    };

    bool point_inside_circle(const point&, const circle&);
    bool point_inside_rect(const point&, const rect&);

}

std::ostream& operator<<(std::ostream&, const tabletopify::vec2&);
std::ostream& operator<<(std::ostream&, const tabletopify::rect&);