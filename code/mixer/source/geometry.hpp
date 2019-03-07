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

    template<typename T>
    constexpr T max(T a, T b)
    {
        if (a > b) return a;
        return b;
    }

    template<typename T>
    constexpr T min(T a, T b)
    {
        if (a < b) return a;
        return b;
    }

    template<typename T>
    struct vec2_
    {
        T x = 0.0;
        T y = 0.0;

        constexpr vec2_() = default;
        constexpr vec2_(const vec2_&) = default;
        constexpr vec2_(vec2_&&) = default;
        
        constexpr vec2_(double x, double y)
        : x(x)
        , y(y)
        { }
        
        constexpr vec2_& operator=(const vec2_&) = default;

        constexpr vec2_ operator+(const vec2_& that) const
        {
            return vec2_(this->x + that.x, this->y + that.y);
        }

        constexpr vec2_ operator-(const vec2_& that) const
        {
            return vec2_(this->x - that.x, this->y - that.y);
        }

        constexpr vec2_ operator*(double d) const
        {
            return vec2_(this->x * d, this->y * d);
        }

        constexpr vec2_& operator+=(const vec2_& that)
        {
            this->x += that.x;
            this->y += that.y;
            return *this;
        }

        constexpr vec2_& operator-=(const vec2_& that)
        {
            this->x -= that.x;
            this->y -= that.y;
            return *this;
        }

        constexpr bool operator==(const vec2_& that) const
        {
            return x == that.x && y == that.y;
        }

        constexpr bool operator!=(const vec2_& that) const
        {
            return x != that.x || y != that.y;
        }

        constexpr static T dot_product(const vec2_& a, const vec2_& b)
        {
            return a.x*b.x + a.y*b.y;
        }

        constexpr T squared_magnitude() const
        {
            return x*x + y*y;
        }

        constexpr operator vec2_<float>() const
        {
            return {(float)x, (float)y};
        };
    };

    template<typename T>
    constexpr vec2_<T> operator*(double d, const vec2_<T>& v)
    {
        return v * d;
    }
    typedef vec2_<double> vec2;
    typedef vec2_<float> vec2f;

    template<typename T>
    struct point_
    {
        vec2_<T> position;
        point_(const vec2_<T>& p)
        : position(p) 
        { }

        constexpr operator point_<float>() const
        {
            return {(vec2f)position};
        }
    };
    typedef point_<double> point;
    typedef point_<float> pointf;

    template<typename T>
    struct rect_
    {
        vec2_<T> position;
        vec2_<T> size;

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

        constexpr operator rect_<float>() const
        {
            return {(vec2f)position, (vec2f)size};
        }
    };
    typedef rect_<double> rect;
    typedef rect_<float> rectf;

    template<typename T>
    struct circle_
    {
        vec2_<T> position;
        T radius;

        constexpr operator circle_<float>() const
        {
            return {(vec2f)position, (float)radius};
        }
    };
    typedef circle_<double> circle;
    typedef circle_<float> circlef;

    template<typename T>
    struct corners_
    {
        T topLeft = 0;
        T topRight = 0;
        T bottomRight = 0;
        T bottomLeft = 0;

        constexpr corners_(T val)
        : topLeft(val)
        , topRight(val)
        , bottomRight(val)
        , bottomLeft(val)
        { }
        constexpr corners_(T tl, T tr, T br, T bl)
        : topLeft(tl)
        , topRight(tr)
        , bottomRight(br)
        , bottomLeft(bl)
        { }

        constexpr corners_() = default;
        constexpr corners_(const corners_&) = default;
        constexpr corners_(corners_&&) = default;

        constexpr corners_& operator=(T val) 
        {
            topLeft = val;
            topRight = val;
            bottomRight = val;
            bottomLeft = val;
            return *this;
        }
        constexpr corners_& operator=(const corners_&) = default;

        constexpr operator corners_<float>() const
        {
            return {(float)topLeft, (float)topRight, (float)bottomRight, (float)bottomLeft};
        }
    };
    typedef corners_<double> corners;
    typedef corners_<float> cornersf;

    // collision types

    constexpr bool point_inside_circle(const point& p, const circle& c)
    {
        auto line = p.position - c.position;
        auto line_len_2 = line.squared_magnitude();

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