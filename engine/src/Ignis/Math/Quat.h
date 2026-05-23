#pragma once

#include "MathUtils.h"
#include "Vec3.h"
#include "Mat4.h"

namespace Ignis::Math
{
    // Unit quaternion: (x,y,z) imaginary, w real
    template<typename T>
    struct Quat
    {
        T x{}, y{}, z{}, w{ T(1) };

        constexpr Quat() = default;
        constexpr Quat(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

        // Hamilton product
        constexpr Quat operator*(const Quat& o) const
        {
            return {
                w*o.x + x*o.w + y*o.z - z*o.y,
                w*o.y - x*o.z + y*o.w + z*o.x,
                w*o.z + x*o.y - y*o.x + z*o.w,
                w*o.w - x*o.x - y*o.y - z*o.z,
            };
        }

        Quat& operator*=(const Quat& o) { *this = *this * o; return *this; }

        T length_sq() const { return x*x + y*y + z*z + w*w; }
        T length()    const { return Math::sqrt(length_sq()); }

        Quat normalized() const
        {
            T len = length();
            return len > T(0) ? Quat{ x/len, y/len, z/len, w/len } : identity();
        }

        constexpr Quat conjugate() const { return { -x, -y, -z, w }; }

        // Rotate a Vec3
        Vec3<T> rotate(const Vec3<T>& v) const
        {
            Vec3<T> qv{ x, y, z };
            Vec3<T> uv  = qv.cross(v);
            Vec3<T> uuv = qv.cross(uv);
            return v + (uv * (T(2) * w)) + (uuv * T(2));
        }

        Mat4<T> to_mat4() const
        {
            T xx = x*x, yy = y*y, zz = z*z;
            T xy = x*y, xz = x*z, yz = y*z;
            T wx = w*x, wy = w*y, wz = w*z;
            // Column-major: cols are (c0r0..c0r3), (c1r0..c1r3), ...
            return {
                T(1)-T(2)*(yy+zz), T(2)*(xy+wz),     T(2)*(xz-wy),     T(0),
                T(2)*(xy-wz),      T(1)-T(2)*(xx+zz), T(2)*(yz+wx),     T(0),
                T(2)*(xz+wy),      T(2)*(yz-wx),      T(1)-T(2)*(xx+yy),T(0),
                T(0),              T(0),               T(0),              T(1),
            };
        }

        // Euler angles (radians): applied ZYX intrinsic (yaw-pitch-roll)
        static Quat from_euler(const Vec3<T>& euler)
        {
            T cx = Math::cos(euler.x * T(0.5));
            T sx = Math::sin(euler.x * T(0.5));
            T cy = Math::cos(euler.y * T(0.5));
            T sy = Math::sin(euler.y * T(0.5));
            T cz = Math::cos(euler.z * T(0.5));
            T sz = Math::sin(euler.z * T(0.5));
            return {
                sx*cy*cz - cx*sy*sz,
                cx*sy*cz + sx*cy*sz,
                cx*cy*sz - sx*sy*cz,
                cx*cy*cz + sx*sy*sz,
            };
        }

        static Quat from_axis_angle(const Vec3<T>& axis, T angle_radians)
        {
            T half = angle_radians * T(0.5);
            T s    = Math::sin(half);
            Vec3<T> a = axis.normalized();
            return { a.x*s, a.y*s, a.z*s, Math::cos(half) };
        }

        static Quat slerp(const Quat& a, Quat b, T t)
        {
            T dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
            if (dot < T(0)) { b = { -b.x, -b.y, -b.z, -b.w }; dot = -dot; }
            if (dot > T(0.9995))
            {
                Quat r{ a.x + t*(b.x-a.x), a.y + t*(b.y-a.y),
                        a.z + t*(b.z-a.z), a.w + t*(b.w-a.w) };
                return r.normalized();
            }
            T theta0 = Math::acos(dot);
            T theta  = theta0 * t;
            T s0     = Math::cos(theta) - dot * Math::sin(theta) / Math::sin(theta0);
            T s1     = Math::sin(theta) / Math::sin(theta0);
            return { s0*a.x + s1*b.x, s0*a.y + s1*b.y, s0*a.z + s1*b.z, s0*a.w + s1*b.w };
        }

        static constexpr Quat identity() { return { T(0), T(0), T(0), T(1) }; }
    };

    template<typename T> Quat<T> normalized(const Quat<T>& q) { return q.normalized(); }
    template<typename T> Quat<T> slerp(const Quat<T>& a, Quat<T> b, T t) { return Quat<T>::slerp(a, b, t); }

    using Quatf      = Quat<float>;
    using Quatd      = Quat<double>;

} // namespace Ignis::Math
