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

        constexpr static color hsl(float h, float s, float l)
        {
            return hsla(h, s, l, 1.0f);
        }

        constexpr static color hsla(float h, float s, float l, float a)
        {
            if (s == 0.0f)
            {
                // achromatic
                return rgba(l, l, l, a);
            }
            else
            {
                auto hue2rgb = [](float p, float q, float t)
                {
                    if(t < 0.0f) t += 1.0f;
                    if(t > 1.0f) t -= 1.0f;
                    if(t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
                    if(t < 1.0f / 2.0f) return q;
                    if(t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t)  * 6.0f;
                    return p;
                };

                float q = (l < 0.5f) ? (l * (1.0 + s)) : (l + s - l * s);
                float p = 2.0f * l - q;

                float r = hue2rgb(p, q, h + 1.0f / 3.0f);
                float g = hue2rgb(p, q, h);
                float b = hue2rgb(p, q, h - 1.0f / 3.0f);

                return rgba(r, g, b, a);
            }
        }

        constexpr static color hsv(float h, float s, float v)
        {
            return hsva(h, s, v, 1.0f);
        }

        constexpr static color hsva(float h, float s, float v, float a)
        {
            int32_t i = static_cast<int32_t>(h * 6.0f);
            float f = h * 6.0f - i;
            float p = v * (1.0f - s);
            float q = v * (1.0f - f * s);
            float t = v * (1.0f - (1.0f - f) * s);
            switch(i % 6)
            {
                case 0: return rgba(v, t, p, a);
                case 1: return rgba(q, v, p, a);
                case 2: return rgba(p, v, t, a);
                case 3: return rgba(p, q, v, a);
                case 4: return rgba(t, p, v, a);
                case 5: return rgba(v, p, q, a);
            }
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