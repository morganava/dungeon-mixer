#pragma once

namespace tabletopify
{
    union color
    {
    public:

        // magenta by default
        constexpr color()
        : raw{1.0f, 0.0f, 1.0f, 1.0f}
        { }

        constexpr color(float r, float g, float b, float a)
        : raw{r,g,b,a}
        { }

        constexpr color(const color& that) = default;

        constexpr static color rgb(uint32_t r, uint32_t g, uint32_t b)
        {
            return rgba(r, g, b, 1.0f);
        }

        constexpr static color rgba(uint32_t r, uint32_t g, uint32_t b, float a)
        {
            MX_THROW_IF_FALSE(r <= 255);
            MX_THROW_IF_FALSE(g <= 255);
            MX_THROW_IF_FALSE(b <= 255);
            MX_THROW_IF_FALSE(a >= 0.0f && a <= 1.0f);
            return color(r / 255.0f, g / 255.0f, b / 255.0f, a);
        }

        operator NVGcolor() const
        {
            return NVGcolor{raw[0], raw[1], raw[2], raw[3]};
        }
    public:
        struct
        {
            float r,g,b,a;
        };
        struct
        {
            float red,green,blue,alpha;
        };
    private:
        float raw[4];
    };

    void to_json(nl::json& js, const color& col);
    void from_json(const nl::json& js, color& col);
}