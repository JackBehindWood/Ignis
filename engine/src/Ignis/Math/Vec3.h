#pragma once

#include "MathUtils.h"

namespace Ignis::Math
{
    template<typename T>
    struct Vec3
    {
        T x{}, y{}, z{};

        constexpr Vec3() = default;
        constexpr Vec3(T x, T y, T z) : x(x), y(y), z(z) {}
        explicit constexpr Vec3(T scalar) : x(scalar), y(scalar), z(scalar) {}

        constexpr Vec3 operator+(const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
        constexpr Vec3 operator-(const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
        constexpr Vec3 operator*(T s)           const { return { x * s,   y * s,   z * s   }; }
        constexpr Vec3 operator/(T s)           const { return { x / s,   y / s,   z / s   }; }
        constexpr Vec3 operator-()              const { return { -x, -y, -z }; }

        Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
        Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
        Vec3& operator*=(T s)           { x *= s;   y *= s;   z *= s;   return *this; }
        Vec3& operator/=(T s)           { x /= s;   y /= s;   z /= s;   return *this; }

        constexpr bool operator==(const Vec3& o) const { return x == o.x && y == o.y && z == o.z; }
        constexpr bool operator!=(const Vec3& o) const { return !(*this == o); }

        constexpr T dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }

        constexpr Vec3 cross(const Vec3& o) const
        {
            return { y * o.z - z * o.y,
                     z * o.x - x * o.z,
                     x * o.y - y * o.x };
        }

        T length_sq() const { return dot(*this); }
        T length()    const { return Math::sqrt(length_sq()); }

        Vec3 normalized() const
        {
            T len = length();
            return len > T(0) ? (*this / len) : Vec3{};
        }

        constexpr T&       operator[](int32_t i)       { return (&x)[i]; }
        constexpr const T& operator[](int32_t i) const { return (&x)[i]; }

        static constexpr Vec3 zero()    { return { T(0), T(0), T(0) }; }
        static constexpr Vec3 one()     { return { T(1), T(1), T(1) }; }
        static constexpr Vec3 up()      { return { T(0), T(1), T(0) }; }
        static constexpr Vec3 right()   { return { T(1), T(0), T(0) }; }
        static constexpr Vec3 forward() { return { T(0), T(0), T(-1) }; }
    };

    template<typename T>
    constexpr Vec3<T> operator*(T s, const Vec3<T>& v) { return v * s; }

    template<typename T> constexpr T       dot(const Vec3<T>& a, const Vec3<T>& b) { return a.dot(b); }
    template<typename T> constexpr Vec3<T> cross(const Vec3<T>& a, const Vec3<T>& b) { return a.cross(b); }
    template<typename T> T                 length(const Vec3<T>& v) { return v.length(); }
    template<typename T> T                 length_sq(const Vec3<T>& v) { return v.length_sq(); }
    template<typename T> Vec3<T>           normalized(const Vec3<T>& v) { return v.normalized(); }

    using Vec3f      = Vec3<float>;
    using Vec3d      = Vec3<double>;

} // namespace Ignis::Math
