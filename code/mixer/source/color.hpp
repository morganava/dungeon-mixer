#pragma once

#include "utilities.hpp"

namespace dm
{
    struct color;
    namespace named_colors
    {
        #define COLOR_ENTRY(NAME, R, G, B, A) extern color NAME;
        #include "color.inl"
        #undef COLOR_ENTRY
    }

    struct color
    {
        // magenta by default
        constexpr color()
        : raw{1.0f, 0.0f, 1.0f, 1.0f}
        { }

        constexpr color(float r, float g, float b, float a)
        : raw{r,g,b,a}
        { }

        constexpr color(const color& that) = default;
        constexpr color(color&&) = default;
        constexpr color& operator=(const color&) = default;
        constexpr color& operator=(color&&) = default;

        constexpr static color rgb(uint32_t r, uint32_t g, uint32_t b)
        {
            return rgba(r, g, b, 1.0f);
        }

        constexpr static color rgba(uint32_t r, uint32_t g, uint32_t b, float a)
        {
            SMOL_THROW_IF_FALSE(r <= 255);
            SMOL_THROW_IF_FALSE(g <= 255);
            SMOL_THROW_IF_FALSE(b <= 255);
            SMOL_THROW_IF_FALSE(a >= 0.0f && a <= 1.0f);
            return color(r / 255.0f, g / 255.0f, b / 255.0f, a);
        }

        constexpr operator NVGcolor() const
        {
            NVGcolor retval;
            retval.r = r;
            retval.g = g;
            retval.b = b;
            retval.a = a;

            return retval;
        }

        #define COLOR_ENTRY(NAME, R, G, B, A) constexpr static const color& NAME = named_colors::NAME;
        #include "color.inl"
        #undef COLOR_ENTRY

        union
        {
            struct
            {
                float r,g,b,a;
            };
            float raw[4];
        };
    };
}

namespace nlohmann
{
    void to_json(nl::json& js, const dm::color& col);
    void from_json(const nl::json& js, dm::color& col);
}

std::ostream& operator<<(std::ostream& out, const dm::color& col);