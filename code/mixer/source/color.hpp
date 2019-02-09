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

        constexpr static color rgb(float r, float g, float b)
        {
            return rgba(r, g, b, 1.0f);
        }

        constexpr static color rgba(float r, float g, float b, float a)
        {
            return color(r, g, b, a);
        }

        // constexpr static color hsl(float h, float s, float l)
        // {
        //     return hsla(h, s, l, 1.0f);
        // }

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