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
    };

    struct rect
    {
        vec2 position;
        vec2 size;
    };    
}

std::ostream& operator<<(std::ostream&, const tabletopify::vec2&);
std::ostream& operator<<(std::ostream&, const tabletopify::rect&);