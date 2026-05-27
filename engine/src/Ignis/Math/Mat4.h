#pragma once

#include "MathUtils.h"
#include "Vec3.h"
#include "Vec4.h"

namespace Ignis::Math
{
// Column-major 4x4 matrix. m[col*4 + row], i.e. m[0..3] = column 0.
template <typename T>
struct Mat4
{
    union
    {
        T m[16]{};
        struct
        {
            Vec4<T> col0, col1, col2, col3;
        };
    };

    constexpr Mat4() = default;

    // Construct from 16 values in column-major order (col0r0, col0r1, col0r2, col0r3, col1r0, ...)
    constexpr Mat4(T c0r0, T c0r1, T c0r2, T c0r3, T c1r0, T c1r1, T c1r2, T c1r3, T c2r0, T c2r1, T c2r2, T c2r3,
                   T c3r0, T c3r1, T c3r2, T c3r3)
    {
        m[0]  = c0r0;
        m[1]  = c0r1;
        m[2]  = c0r2;
        m[3]  = c0r3;
        m[4]  = c1r0;
        m[5]  = c1r1;
        m[6]  = c1r2;
        m[7]  = c1r3;
        m[8]  = c2r0;
        m[9]  = c2r1;
        m[10] = c2r2;
        m[11] = c2r3;
        m[12] = c3r0;
        m[13] = c3r1;
        m[14] = c3r2;
        m[15] = c3r3;
    }

    constexpr Vec4<T>& operator[](int32_t col)
    {
        return (&col0)[col];
    }

    constexpr const Vec4<T>& operator[](int32_t col) const
    {
        return (&col0)[col];
    }

    // Element access: at(row, col)
    constexpr T& at(int32_t row, int32_t col)
    {
        return m[col * 4 + row];
    }
    constexpr const T& at(int32_t row, int32_t col) const
    {
        return m[col * 4 + row];
    }

    // Raw point32_ter for GPU upload
    const T* data() const
    {
        return m;
    }
    T* data()
    {
        return m;
    }

    // Extract a row as Vec4
    constexpr Vec4<T> row(int32_t r) const
    {
        return {m[r], m[4 + r], m[8 + r], m[12 + r]};
    }

    // Extract a column as Vec4
    constexpr Vec4<T> col(int32_t c) const
    {
        return {m[c * 4], m[c * 4 + 1], m[c * 4 + 2], m[c * 4 + 3]};
    }

    // Matrix multiply (this * rhs)
    Mat4 operator*(const Mat4& rhs) const
    {
        Mat4 res{};
        for (int32_t c = 0; c < 4; ++c)
        {
            for (int32_t r = 0; r < 4; ++r)
            {
                for (int32_t k = 0; k < 4; ++k)
                {
                    res.m[c * 4 + r] += m[k * 4 + r] * rhs.m[c * 4 + k];
                }
            }
        }
        return res;
    }

    Mat4& operator*=(const Mat4& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    // Transform a Vec4
    constexpr Vec4<T> operator*(const Vec4<T>& v) const
    {
        return {
            m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w,
            m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
            m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w,
            m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w,
        };
    }

    Mat4 transposed() const
    {
        return {
            m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15],
        };
    }

    static constexpr Mat4 identity()
    {
        return {
            T(1), T(0), T(0), T(0), T(0), T(1), T(0), T(0), T(0), T(0), T(1), T(0), T(0), T(0), T(0), T(1),
        };
    }
};

template <typename T>
constexpr Mat4<T> translate(const Vec3<T>& t)
{
    Mat4<T> r = Mat4<T>::identity();
    r.m[12]   = t.x;
    r.m[13]   = t.y;
    r.m[14]   = t.z;
    return r;
}

template <typename T>
constexpr Mat4<T> scale(const Vec3<T>& s)
{
    Mat4<T> r = Mat4<T>::identity();
    r.m[0]    = s.x;
    r.m[5]    = s.y;
    r.m[10]   = s.z;
    return r;
}

// Right-handed perspective projection (depth range [0,1] for Metal/Vulkan)
template <typename T>
Mat4<T> perspective(T fov_y_radians, T aspect, T near_z, T far_z)
{
    T       tan_half = Math::tan(fov_y_radians / T(2));
    Mat4<T> r{};
    r.m[0]  = T(1) / (aspect * tan_half);
    r.m[5]  = T(1) / tan_half;
    r.m[10] = far_z / (near_z - far_z);
    r.m[11] = T(-1);
    r.m[14] = -(far_z * near_z) / (far_z - near_z);
    return r;
}

// Right-handed look-at view matrix
template <typename T>
Mat4<T> look_at(const Vec3<T>& eye, const Vec3<T>& center, const Vec3<T>& up)
{
    Vec3<T> f = normalized(center - eye);
    Vec3<T> r = normalized(cross(f, up));
    Vec3<T> u = cross(r, f);

    Mat4<T> res = Mat4<T>::identity();
    res.m[0]    = r.x;
    res.m[4]    = r.y;
    res.m[8]    = r.z;
    res.m[1]    = u.x;
    res.m[5]    = u.y;
    res.m[9]    = u.z;
    res.m[2]    = -f.x;
    res.m[6]    = -f.y;
    res.m[10]   = -f.z;
    res.m[12]   = -dot(r, eye);
    res.m[13]   = -dot(u, eye);
    res.m[14]   = dot(f, eye);
    return res;
}

template <typename T>
Mat4<T> transposed(const Mat4<T>& mat)
{
    return mat.transposed();
}

using Mat4f = Mat4<float>;
using Mat4d = Mat4<double>;
} // namespace Ignis::Math
