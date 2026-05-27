#pragma once

#include "MathUtils.h"

namespace Ignis::Math
{
template <typename T>
struct Vec2
{
    T x{}, y{};

    constexpr Vec2() = default;
    constexpr Vec2(T x, T y)
        : x(x),
          y(y)
    {
    }
    explicit constexpr Vec2(T scalar)
        : x(scalar),
          y(scalar)
    {
    }

    constexpr Vec2 operator+(const Vec2& o) const
    {
        return {x + o.x, y + o.y};
    }
    constexpr Vec2 operator-(const Vec2& o) const
    {
        return {x - o.x, y - o.y};
    }
    constexpr Vec2 operator*(T s) const
    {
        return {x * s, y * s};
    }
    constexpr Vec2 operator/(T s) const
    {
        return {x / s, y / s};
    }
    constexpr Vec2 operator-() const
    {
        return {-x, -y};
    }

    Vec2& operator+=(const Vec2& o)
    {
        x += o.x;
        y += o.y;
        return *this;
    }
    Vec2& operator-=(const Vec2& o)
    {
        x -= o.x;
        y -= o.y;
        return *this;
    }
    Vec2& operator*=(T s)
    {
        x *= s;
        y *= s;
        return *this;
    }
    Vec2& operator/=(T s)
    {
        x /= s;
        y /= s;
        return *this;
    }

    constexpr bool operator==(const Vec2& o) const
    {
        return x == o.x && y == o.y;
    }
    constexpr bool operator!=(const Vec2& o) const
    {
        return !(*this == o);
    }

    constexpr T dot(const Vec2& o) const
    {
        return x * o.x + y * o.y;
    }

    T length_sq() const
    {
        return dot(*this);
    }
    T length() const
    {
        return Math::sqrt(length_sq());
    }

    Vec2 normalized() const
    {
        T len = length();
        return len > T(0) ? (*this / len) : Vec2{};
    }

    constexpr T& operator[](int32_t i)
    {
        return (&x)[i];
    }
    constexpr const T& operator[](int32_t i) const
    {
        return (&x)[i];
    }

    static constexpr Vec2 zero()
    {
        return {T(0), T(0)};
    }
    static constexpr Vec2 one()
    {
        return {T(1), T(1)};
    }
};

template <typename T>
constexpr Vec2<T> operator*(T s, const Vec2<T>& v)
{
    return v * s;
}

template <typename T>
constexpr T dot(const Vec2<T>& a, const Vec2<T>& b)
{
    return a.dot(b);
}

template <typename T>
T length(const Vec2<T>& v)
{
    return v.length();
}

template <typename T>
T length_sq(const Vec2<T>& v)
{
    return v.length_sq();
}

template <typename T>
Vec2<T> normalized(const Vec2<T>& v)
{
    return v.normalized();
}

using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;

} // namespace Ignis::Math
