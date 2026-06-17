#pragma once

namespace Ignis::Math
{

struct LinearColour
{
    union
    {
        struct
        {
            float r, g, b, a;
        };
        float data[4];
    };

    constexpr LinearColour()
        : r(1.0f),
          g(1.0f),
          b(1.0f),
          a(1.0f)
    {
    }
    constexpr LinearColour(float r, float g, float b, float a = 1.0f)
        : r(r),
          g(g),
          b(b),
          a(a)
    {
    }

    constexpr bool operator==(const LinearColour& o) const
    {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
    constexpr bool operator!=(const LinearColour& o) const
    {
        return !(*this == o);
    }
};

static_assert(sizeof(LinearColour) == 16);

} // namespace Ignis::Math
