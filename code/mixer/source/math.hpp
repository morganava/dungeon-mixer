#pragma once

namespace dm
{
    // constants
    template<typename T>
    struct constants
    {
        constexpr static T pi = 3.14159265358979323846;
    };

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

    template<typename T>
    constexpr T lerp(T a, T b, T t)
    {
        return a + t * (b - a);
    }
}