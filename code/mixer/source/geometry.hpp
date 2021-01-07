#pragma once

#include "utilities.hpp"
#include "math.hpp"

namespace dm
{
    // vector 2

    template<typename T>
    struct vec2_
    {
        T x = T(0);
        T y = T(0);

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

        constexpr T magnitude() const
        {
            return std::sqrt(squared_magnitude());
        }

        constexpr operator vec2_<float>() const
        {
            return {(float)x, (float)y};
        }

        constexpr static const vec2_ zero = {0,0};
    };

    template<typename T>
    constexpr vec2_<T> operator*(double d, const vec2_<T>& v)
    {
        return v * d;
    }
    typedef vec2_<double> vec2;
    typedef vec2_<float> vec2f;

    void from_json(const nl::json&, vec2&);

    // vector 3

    template<typename T>
    struct vec3_
    {
        T x = T(0);
        T y = T(0);
        T z = T(0);

        constexpr vec3_() = default;
        constexpr vec3_(const vec3_&) = default;
        constexpr vec3_(vec3_&&) = default;

        constexpr vec3_(T x, T y, T z)
        : x(x)
        , y(y)
        , z(z)
        { }

        constexpr explicit vec3_(T val)
        : x(val)
        , y(val)
        , z(val)
        { }

        constexpr vec3_(const vec2_<T>& val)
        : x(val.x)
        , y(val.y)
        , z(0)
        { }

        constexpr vec3_& operator=(const vec3_&) = default;

        constexpr vec3_ operator+(const vec3_& that) const
        {
            return vec3_(this->x + that.x, this->y + that.y, this->z + that.z);
        }

        constexpr vec3_ operator-(const vec3_& that) const
        {
            return vec3_(this->x - that.x, this->y - that.y, this->z - that.z);
        }

        constexpr vec3_ operator*(T d) const
        {
            return vec3_(this->x * d, this->y * d, this->z * d);
        }

        friend constexpr vec3_ operator*(T d, const vec3_& v)
        {
            return v * d;
        }

        constexpr vec3_ operator/(T d) const
        {
            return vec3_(this->x / d, this->y / d, this->z * d);
        }

        constexpr vec3_& operator+=(const vec3_& that)
        {
            this->x += that.x;
            this->y += that.y;
            this->z += that.z;
            return *this;
        }

        constexpr vec3_& operator-=(const vec3_& that)
        {
            this->x -= that.x;
            this->y -= that.y;
            this->z -= that.z;
            return *this;
        }

        constexpr bool operator==(const vec3_& that) const
        {
            return x == that.x && y == that.y && z == that.z;
        }

        constexpr bool operator!=(const vec3_& that) const
        {
            return x != that.x || y != that.y || z != that.z;
        }

        constexpr static T dot_product(const vec3_& a, const vec3_& b)
        {
            return a.x*b.x + a.y*b.y + a.z*b.z;
        }

        constexpr T squared_magnitude() const
        {
            return x*x + y*y + z*z;
        }

        constexpr T magnitude() const
        {
            return std::sqrt(squared_magnitude());
        }

        constexpr operator vec3_<float>() const
        {
            return {(float)x, (float)y, (float)z};
        }

        constexpr static const vec3_ zero = {0,0,0};

        constexpr T& operator[](size_t idx)
        {
            switch(idx)
            {
                case 0: return x;
                case 1: return y;
                case 2: return z;
            }
            return *static_cast<T*>(nullptr);
        }

        constexpr T operator[](size_t idx) const
        {
            switch(idx)
            {
                case 0: return x;
                case 1: return y;
                case 2: return z;
            }
            return *static_cast<T*>(nullptr);
        }

        // convert to SFML vector
        operator sf::Vector3f() const
        {
            return {(float)x, (float)y, (float)z};
        }
    };


    typedef vec3_<double> vec3;
    typedef vec3_<float> vec3f;

    void from_json(const nl::json&, vec3&);

    template<typename T, size_t ROWS, size_t COLS>
    struct matrix_
    {
        constexpr matrix_(std::initializer_list<T> lst)
        {
            std::copy(lst.begin(), lst.end(), elements);
        }

        constexpr matrix_(const matrix_&) = default;
        constexpr matrix_(matrix_&&) = default;

        // multiple an LxM matrix by an MxN matrix to get an LxN marix
        template<size_t N>
        constexpr matrix_<T,COLS,N> operator*(const matrix_<T,COLS,N>& B)
        {
            constexpr size_t L = ROWS;
            constexpr size_t M = COLS;

            const auto& A = *this;

            matrix_<T,L,N> C;
            for(auto i = 0; i < L; ++i)
            {
                for(auto j = 0; j < N; ++j)
                {
                    auto& c_ij = C(i,j);
                    c_ij = T(0);
                    for(auto k = 0; k < M; ++k)
                    {
                        c_ij += A(i,k) * B(k,j);
                    }
                }
            }
            return C;
        }

        template<size_t I, size_t J>
        constexpr T& get()
        {
            static_assert(I < ROWS);
            static_assert(J < COLS);

            return elements[I * COLS + J];
        }

        constexpr T& operator()(size_t i, size_t j)
        {
            MX_ASSERT(i < ROWS);
            MX_ASSERT(j < COLS);

            return elements[i * COLS + j];
        }

        constexpr T operator()(size_t i, size_t j) const
        {
            MX_ASSERT(i < ROWS);
            MX_ASSERT(j < COLS);

            return elements[i * COLS + j];
        }
    private:
        T elements[ROWS*COLS] = {0};
    };

    template<typename T>
    struct matrix3x3_ : public matrix_<T, 3, 3>
    {
        using matrix_<T,3,3>::matrix_;
        using matrix_<T,3,3>::operator*;

        constexpr friend vec3_<T> operator*(const vec3_<T>& V, const matrix3x3_& M)
        {
            vec3_<T> R;
            for(auto i = 0; i < 3; ++i)
            {
                auto& r = R[i];
                for(auto j = 0; j < 3; ++j)
                {
                    r += V[j] * M(i,j);
                }
            }
            return R;
        }
    };
    typedef matrix3x3_<double> matrix3x3;
    typedef matrix3x3_<float> matrix3x3f;

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

        constexpr rect_() = default;

        constexpr rect_(const vec2_<T>& position, const vec2_<T>& size)
        : position(position)
        , size(size)
        { }

        constexpr T left() const
        {
            return position.x;
        }

        constexpr T right() const
        {
            return position.x + size.x;
        }

        constexpr T top() const
        {
            return position.y;
        }

        constexpr T bottom() const
        {
            return position.y + size.y;
        }

        constexpr vec2_<T> center() const
        {
            return position + (size / T(2));
        }

        constexpr T width() const
        {
            return size.x;
        }

        constexpr T height() const
        {
            return size.y;
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

        constexpr vec2_<T> center() const
        {
            return position;
        }

        constexpr T left() const
        {
            return position.x - radius;
        }

        constexpr T right() const
        {
            return position.x + radius;
        }

        constexpr T top() const
        {
            return position.y - radius;
        }

        constexpr T bottom() const
        {
            return position.y + radius;
        }

        constexpr T diameter() const
        {
            return radius * 2;
        }

        constexpr rect_<T> bounding_box() const
        {
            return rect_<T>{
                {left(), right()},
                {diameter(), diameter()}
            };
        }

        constexpr operator circle_<float>() const
        {
            return {(vec2f)position, (float)radius};
        }
    };
    typedef circle_<double> circle;
    typedef circle_<float> circlef;

    // ellipse

    template<typename T>
    struct ellipse_
    {
        vec2_<T> position;
        vec2_<T> radius;

        constexpr operator ellipse_<float>() const
        {
            return {(vec2f)position, (vec2f)radius};
        }
    };
    typedef ellipse_<double> ellipse;
    typedef ellipse_<float> ellipsef;

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
        constexpr corners_ operator-(T val) const
        {
            auto retval = *this;
            for(auto& r : retval.raw)
            {
                r -= val;
            }
            return retval;
        }

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
                vec2_<T>{padding.left, padding.top};

            auto bottomRight =
                outerBounds.bottom_right() -
                vec2_<T>{margin.right, margin.bottom} -
                vec2_<T>{borderSize, borderSize} -
                vec2_<T>{padding.right, padding.bottom};

            const auto& position = topLeft;
            auto size = bottomRight - topLeft;

            return rect_<T> { position, size };
        }

        constexpr rect_<T> inner_bounds() const
        {
            return child_bounds();
        }

        constexpr rect_<T> border_inner_bounds() const
        {
            auto topLeft =
                outerBounds.top_left() +
                vec2_<T>{margin.left, margin.top} +
                vec2_<T>{borderSize, borderSize};

            auto bottomRight =
                outerBounds.bottom_right() -
                vec2_<T>{margin.right, margin.bottom} -
                vec2_<T>{borderSize, borderSize};

            const auto& position = topLeft;
            auto size = bottomRight - topLeft;

            return rect_<T> { position, size };
        }

        constexpr rect_<T> border_outer_bounds() const
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

    constexpr bool point_inside_ellipse(const point& p, const ellipse& e)
    {
        // normalized point
        auto np = p.position - e.position;
        np.x /= e.radius.x;
        np.y /= e.radius.y;

        return np.squared_magnitude() <= 1.0;
    }

    constexpr bool point_inside_rect(const point& p, const rect& r)
    {
        return (p.position.x > r.position.x &&
                p.position.x < (r.position.x + r.size.x) &&
                p.position.y > r.position.y &&
                p.position.y < (r.position.y + r.size.y));
    }

    constexpr bool point_inside_rounded_rect(const point& p, const rect& r, const corners& rad)
    {
        if (!point_inside_rect(p, r))
        {
            return false;
        }

        // top left corner
        if (rad.topLeft > 0.0)
        {
            vec2 topLeftBoxSize = {
                min(rad.topLeft, r.width() / 2.0),
                min(rad.topLeft, r.height() / 2.0)
            };
            rect topLeftBox = {
                r.top_left(),
                topLeftBoxSize
            };
            if (point_inside_rect(p, topLeftBox))
            {
                vec2 position = topLeftBox.bottom_right();
                const auto& radius = topLeftBoxSize;

                return point_inside_ellipse(p, {position, radius});
            }
        }

        // top right corner
        if (rad.topRight > 0.0)
        {
            vec2 topRightBoxSize = {
                min(rad.topRight, r.width() / 2.0),
                min(rad.topRight, r.height() / 2.0)
            };
            rect topRightBox = {
                r.top_right() + vec2{-topRightBoxSize.x, 0.0},
                topRightBoxSize
            };
            if (point_inside_rect(p, topRightBox))
            {
                vec2 position = topRightBox.bottom_left();
                const auto& radius = topRightBoxSize;

                return point_inside_ellipse(p, {position, radius});
            }
        }

        // bottom right corner
        if (rad.bottomRight > 0.0)
        {
            vec2 bottomRightBoxSize = {
                min(rad.bottomRight, r.width() / 2.0),
                min(rad.bottomRight, r.height() / 2.0),
            };
            rect bottomRightBox = {
                r.bottom_right() + vec2{-bottomRightBoxSize.x, -bottomRightBoxSize.y},
                bottomRightBoxSize
            };
            if (point_inside_rect(p, bottomRightBox))
            {
                vec2 position = bottomRightBox.top_left();
                const auto& radius = bottomRightBoxSize;

                return point_inside_ellipse(p, {position, radius});
            }
        }

        // bottom left corner
        if (rad.bottomLeft > 0.0)
        {
            vec2 bottomLeftBoxSize = {
                min(rad.bottomLeft, r.width() / 2.0),
                min(rad.bottomLeft, r.height() / 2.0)
            };
            rect bottomLeftBox = {
                r.bottom_left() + vec2{0.0, -bottomLeftBoxSize.y},
                bottomLeftBoxSize
            };
            if (point_inside_rect(p, bottomLeftBox))
            {
                vec2 position = bottomLeftBox.top_right();
                const auto& radius = bottomLeftBoxSize;

                return point_inside_ellipse(p, {position, radius});
            }
        }

        return true;
    }

    constexpr bool rect_intersects_rect(const rect& r1, const rect& r2)
    {
        if (r1.right() < r2.left() || r1.left() > r2.right()) return false;
        if (r1.bottom() < r2.top() || r1.top() > r2.bottom()) return false;
        return true;
    }

    template<typename T>
    constexpr rect_<T> get_intersecting_rect(const rect_<T>& r1, const rect_<T>& r2)
    {
        if (!rect_intersects_rect(r1, r2)) return {};

        auto left = std::max(r1.left(), r2.left());
        auto right = std::min(r1.right(), r2.right());
        auto top = std::max(r1.top(), r2.top());
        auto bottom = std::min(r1.bottom(), r2.bottom());

        return {{left, top}, {right - left, bottom - top}};
    }
}

template<typename T>
std::ostream& operator<<(std::ostream& out, const dm::vec2_<T>& v)
{
    return out << "{ x : " << v.x << ", y : " << v.y << " }";
}
template<typename T>
std::ostream& operator<<(std::ostream& out, const dm::vec3_<T>& v)
{
    return out << "{ x : " << v.x << ", y : " << v.y << ", z : " << v.z << " }";
}
template<typename T>
std::ostream& operator<<(std::ostream& out, const dm::rect_<T>& r)
{
    return out << "{ position : " << r.position << " size : " << r.size << " }";
}
std::ostream& operator<<(std::ostream&, const dm::corners&);
std::ostream& operator<<(std::ostream&, const dm::edges&);
