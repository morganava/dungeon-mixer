#pragma once

namespace tabletopify
{
    // math functions

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

    // vector 2

    template<typename T>
    struct vec2_
    {
        T x = 0.0;
        T y = 0.0;

        constexpr vec2_() = default;
        constexpr vec2_(const vec2_&) = default;
        constexpr vec2_(vec2_&&) = default;

        constexpr vec2_(T x, T y)
        : x(x)
        , y(y)
        { }

        constexpr explicit vec2_(T val)
        : x(val)
        , y(val)
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

        constexpr vec2_ operator*(T d) const
        {
            return vec2_(this->x * d, this->y * d);
        }

        constexpr vec2_ operator/(T d) const
        {
            return vec2_(this->x / d, this->y / d);
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

    // point

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

    // rectangle

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

        constexpr vec2_<T> top_left() const
        {
            return position;
        }

        constexpr vec2_<T> top_right() const
        {
            return position + vec2_<T>{size.x, T{0}};
        }

        constexpr vec2_<T> bottom_left() const
        {
            return position + vec2_<T>{T{0}, size.y};
        }

        constexpr vec2_<T> bottom_right() const
        {
            return position + size;
        }

        constexpr operator rect_<float>() const
        {
            return {(vec2f)position, (vec2f)size};
        }
    };
    typedef rect_<double> rect;
    typedef rect_<float> rectf;

    // circle

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

    // corners (for rounded rects for instance)

    template<typename T>
    struct corners_
    {
        union
        {
            struct
            {
                T topLeft = 0;
                T topRight = 0;
                T bottomRight = 0;
                T bottomLeft = 0;
            };
            T raw[4];
        };
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
            for(auto& r : raw)
            {
                r = val;
            }
            return *this;
        }
        constexpr corners_& operator=(const corners_&) = default;
        constexpr corners_& operator=(corners_&&) = default;

        constexpr operator corners_<float>() const
        {
            return {(float)topLeft, (float)topRight, (float)bottomRight, (float)bottomLeft};
        }
    };
    typedef corners_<double> corners;
    typedef corners_<float> cornersf;

    void from_json(const nl::json&, corners&);

    template<typename T>
    struct edges_
    {
        union
        {
            struct
            {
                T top = 0;
                T right = 0;
                T bottom = 0;
                T left = 0;
            };
            T raw[4];
        };

        constexpr edges_(T val)
        : top(val)
        , right(val)
        , bottom(val)
        , left(val)
        { }
        constexpr edges_(T t, T r, T b, T l)
        : top(t)
        , right(r)
        , bottom(b)
        , left(l)
        { }

        constexpr edges_() = default;
        constexpr edges_(const edges_&) = default;
        constexpr edges_(edges_&&) = default;

        constexpr edges_& operator=(T val)
        {
            for(auto& r : raw)
            {
                r = val;
            }
            return *this;
        }
        constexpr edges_& operator=(const edges_&) = default;
        constexpr edges_& operator=(edges_&&) = default;

        constexpr operator edges_<float>() const
        {
            return {static_cast<float>(top), static_cast<float>(right), static_cast<float>(bottom), static_cast<float>(left)};
        }
    };
    typedef edges_<double> edges;
    typedef edges_<float> edgesf;

    void from_json(const nl::json&, edges&);

    template<typename T>
    struct box_layout_
    {
        rect_<T> outerBounds;
        edges_<T> margin;
        T borderSize;
        corners_<T> borderRadius;
        edges_<T> padding;

        constexpr rect_<T> child_bounds() const
        {
            auto topLeft =
                outerBounds.top_left() +
                vec2_<T>{margin.left, margin.top} +
                vec2_<T>{borderSize, borderSize} +
                vec2_<T>{padding.left, padding.right};

            auto bottomRight =
                outerBounds.bottom_right() -
                vec2_<T>{margin.right, margin.bottom} -
                vec2_<T>{borderSize, borderSize} -
                vec2_<T>{padding.right, padding.bottom};

            const auto& position = topLeft;
            auto size = bottomRight - topLeft;

            return rect_<T> { position, size };
        }

        constexpr rect_<T> border_bounds() const
        {
            auto topLeft =
                outerBounds.top_left() +
                vec2_<T>{margin.left, margin.top};

            auto bottomRight =
                outerBounds.bottom_right() -
                vec2_<T>{margin.right, margin.bottom};

            const auto& position = topLeft;
            auto size = bottomRight - topLeft;

            return rect_<T> { position, size };
        }
    };
    typedef box_layout_<double> box_layout;
    typedef box_layout_<float> box_layoutf;

    // collision checks

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

    // todo cubic bezier approximation when a radius is larger than half the rect width/height
    constexpr bool point_inside_rounded_rect(const point& p, const rect& r, const corners& rad)
    {
        if (!point_inside_rect(p, r))
        {
            return false;
        }

        rect topLeftBox = {r.top_left(), vec2{rad.topLeft}};
        if (point_inside_rect(p, topLeftBox))
        {
            circle topLeftCircle = {topLeftBox.bottom_right(), rad.topLeft};
            return point_inside_circle(p, topLeftCircle);
        }

        rect topRightBox = {r.top_right() + vec2{-rad.topRight, 0}, vec2{rad.topRight}};
        if (point_inside_rect(p, topRightBox))
        {
            circle topRightCircle = {topRightBox.bottom_left(), rad.topRight};
            return point_inside_circle(p, topRightCircle);
        }

        rect bottomRightBox = {r.bottom_right() + vec2{-rad.bottomRight, -rad.bottomRight}, vec2{rad.bottomRight}};
        if (point_inside_rect(p, bottomRightBox))
        {
            circle bottomRightCircle = {bottomRightBox.top_left(), rad.bottomRight};
            return point_inside_circle(p, bottomRightCircle);
        }

        rect bottomLeftBox = {r.bottom_left() + vec2{0, -rad.bottomRight}, vec2{rad.bottomLeft}};
        if (point_inside_rect(p, bottomLeftBox))
        {
            circle bottomLeftCircle = {bottomLeftBox.top_right(), rad.bottomLeft};
            return point_inside_circle(p, bottomLeftCircle);
        }

        return true;
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