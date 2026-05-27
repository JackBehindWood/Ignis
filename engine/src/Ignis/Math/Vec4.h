#pragma once

#include "MathUtils.h"
#include "Vec3.h"

namespace Ignis::Math
{
template <typename T>
struct Vec4
{
    T x{}, y{}, z{}, w{};

    constexpr Vec4() = default;
    constexpr Vec4(T x, T y, T z, T w)
        : x(x),
          y(y),
          z(z),
          w(w)
    {
    }
    explicit constexpr Vec4(T scalar)
        : x(scalar),
          y(scalar),
          z(scalar),
          w(scalar)
    {
    }
    constexpr Vec4(Vec3<T> xyz, T w)
        : x(xyz.x),
          y(xyz.y),
          z(xyz.z),
          w(w)
    {
    }

    constexpr Vec4 operator+(const Vec4& o) const
    {
        return {x + o.x, y + o.y, z + o.z, w + o.w};
    }
    constexpr Vec4 operator-(const Vec4& o) const
    {
        return {x - o.x, y - o.y, z - o.z, w - o.w};
    }
    constexpr Vec4 operator*(T s) const
    {
        return {x * s, y * s, z * s, w * s};
    }
    constexpr Vec4 operator/(T s) const
    {
        return {x / s, y / s, z / s, w / s};
    }
    constexpr Vec4 operator-() const
    {
        return {-x, -y, -z, -w};
    }

    Vec4& operator+=(const Vec4& o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        w += o.w;
        return *this;
    }
    Vec4& operator-=(const Vec4& o)
    {
        x -= o.x;
        y -= o.y;
        z -= o.z;
        w -= o.w;
        return *this;
    }
    Vec4& operator*=(T s)
    {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }
    Vec4& operator/=(T s)
    {
        x /= s;
        y /= s;
        z /= s;
        w /= s;
        return *this;
    }

    constexpr bool operator==(const Vec4& o) const
    {
        return x == o.x && y == o.y && z == o.z && w == o.w;
    }
    constexpr bool operator!=(const Vec4& o) const
    {
        return !(*this == o);
    }

    constexpr T dot(const Vec4& o) const
    {
        return x * o.x + y * o.y + z * o.z + w * o.w;
    }

    T length_sq() const
    {
        return dot(*this);
    }
    T length() const
    {
        return Math::sqrt(length_sq());
    }

    Vec4 normalized() const
    {
        T len = length();
        return len > T(0) ? (*this / len) : Vec4{};
    }

    constexpr Vec3<T> xyz() const
    {
        return {x, y, z};
    }

    constexpr T& operator[](int32_t i)
    {
        return (&x)[i];
    }
    constexpr const T& operator[](int32_t i) const
    {
        return (&x)[i];
    }
};

template <typename T>
constexpr Vec4<T> operator*(T s, const Vec4<T>& v)
{
    return v * s;
}

template <typename T>
constexpr T dot(const Vec4<T>& a, const Vec4<T>& b)
{
    return a.dot(b);
}
template <typename T>
T length(const Vec4<T>& v)
{
    return v.length();
}
template <typename T>
T length_sq(const Vec4<T>& v)
{
    return v.length_sq();
}
template <typename T>
Vec4<T> normalized(const Vec4<T>& v)
{
    return v.normalized();
}

using Vec4f = Vec4<float>;
using Vec4d = Vec4<double>;

} // namespace Ignis::Math
