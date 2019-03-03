#pragma once

namespace tabletopify
{
    template<typename T>
    constexpr T clamp(const T& min, const T& val, const T& max)
    {
        if (val <= min) return min;
        if (val >= max) return max;
        return val;
    }

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

        constexpr vec2& operator+=(const vec2& that)
        {
            this->x += that.x;
            this->y += that.y;
            return *this;
        }

        constexpr vec2& operator-=(const vec2& that)
        {
            this->x -= that.x;
            this->y -= that.y;
            return *this;
        }

        constexpr bool operator==(const vec2& that) const
        {
            return x == that.x && y == that.y;
        }

        constexpr bool operator!=(const vec2& that) const
        {
            return x != that.x || y != that.y;
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

    constexpr vec2 operator*(double d, const vec2& v)
    {
        return v * d;
    }

    struct point
    {
        vec2 position;
        point(const vec2& p)
        : position(p) 
        { }
    };

    struct rect
    {
        vec2 position;
        vec2 size;

        constexpr double left() const
        {
            return position.x;
        }

        constexpr double right() const
        {
            return position.x + size.x;
        }

        constexpr double top() const
        {
            return position.y;
        }

        constexpr double bottom() const
        {
            return position.y + size.y;
        }
    };

    struct circle
    {
        vec2 position;
        double radius;
    };

    constexpr bool point_inside_circle(const point& p, const circle& c)
    {
        vec2 line = p.position - c.position;
        double line_len_2 = line.squared_magnitude();

        return line_len_2 <= (c.radius * c.radius);
    }

    constexpr bool point_inside_rect(const point& p, const rect& r)
    {
        return (p.position.x > r.position.x &&
                p.position.x < (r.position.x + r.size.x) &&
                p.position.y > r.position.y &&
                p.position.y < (r.position.y + r.size.y));
    }

    constexpr bool rect_intersects_rect(const rect& r1, const rect& r2)
    {
        if (r1.right() < r2.left() || r1.left() > r2.right()) return false;
        if (r1.bottom() < r2.top() || r1.top() > r2.bottom()) return false;
        return true;
    }

    constexpr rect get_intersecting_rect(const rect& r1, const rect& r2)
    {
        if (!rect_intersects_rect(r1, r2)) return {};

        auto left = std::max(r1.left(), r2.left());
        auto right = std::min(r1.right(), r2.right());
        auto top = std::max(r1.top(), r2.top());
        auto bottom = std::min(r1.bottom(), r2.bottom());

        return {{left, top}, {right - left, bottom - top}};
    }
}

std::ostream& operator<<(std::ostream&, const tabletopify::vec2&);
std::ostream& operator<<(std::ostream&, const tabletopify::rect&);