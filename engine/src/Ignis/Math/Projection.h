#pragma once

#include "Mat4.h"
#include "MathUtils.h"

// NOTE: all of the code is for a left handed coordinate system with a column major matrix!

namespace Ignis::Math
{

template <typename T>
Mat4<T> ortho(T left, T right, T bottom, T top, T near_z, T far_z)
{
    const T rl = right - left;
    const T tb = top - bottom;
    const T fn = far_z - near_z;
    Mat4<T> r{};
    r.m[0]  = T(2) / rl;
    r.m[5]  = T(2) / tb;
    r.m[10] = T(1) / fn;
    r.m[12] = -(right + left) / rl;
    r.m[13] = -(top + bottom) / tb;
    r.m[14] = -near_z / fn;
    r.m[15] = T(1);
    return r;
}

template <typename T>
Mat4<T> inverse_perspective(T fov_y_radians, T aspect, T near_z, T far_z)
{
    const T tan_half = Math::tan(fov_y_radians / T(2));
    Mat4<T> r{};
    r.m[0]  = aspect * tan_half;
    r.m[5]  = tan_half;
    r.m[11] = -(far_z - near_z) / (far_z * near_z);
    r.m[14] = T(1);
    r.m[15] = T(1) / near_z;
    return r;
}

template <typename T>
Mat4<T> inverse_ortho(T left, T right, T bottom, T top, T near_z, T far_z)
{
    Mat4<T> r{};
    r.m[0]  = (right - left) / T(2);
    r.m[5]  = (top - bottom) / T(2);
    r.m[10] = far_z - near_z;
    r.m[12] = (right + left) / T(2);
    r.m[13] = (top + bottom) / T(2);
    r.m[14] = near_z;
    r.m[15] = T(1);
    return r;
}

struct ViewBasis
{
    Vec3<float> right;
    Vec3<float> up;
    Vec3<float> forward;
};

inline ViewBasis extract_view_basis(const Mat4f& v)
{
    return {
        {v.m[0], v.m[4], v.m[8]},
        {v.m[1], v.m[5], v.m[9]},
        {v.m[2], v.m[6], v.m[10]},
    };
}

struct PerspectiveParams
{
    float fov_y;
    float aspect;
    float near_z;
    float far_z;
};

struct OrthoParams
{
    float left, right, bottom, top, near_z, far_z;
};

inline PerspectiveParams extract_perspective_params(const Mat4f& p)
{
    PerspectiveParams out;
    out.fov_y  = 2.0f * Math::atan(1.0f / p.m[5]);
    out.aspect = p.m[5] / p.m[0];
    out.near_z = -p.m[14] / p.m[10];
    out.far_z  = -p.m[14] / (p.m[10] - 1.0f);
    return out;
}

inline OrthoParams extract_ortho_params(const Mat4f& p)
{
    const float rl    = 2.0f / p.m[0];
    const float tb    = 2.0f / p.m[5];
    const float sum_x = -p.m[12] * rl;
    const float sum_y = -p.m[13] * tb;
    OrthoParams out;
    out.right  = (sum_x + rl) * 0.5f;
    out.left   = (sum_x - rl) * 0.5f;
    out.top    = (sum_y + tb) * 0.5f;
    out.bottom = (sum_y - tb) * 0.5f;
    out.near_z = -p.m[14] / p.m[10];
    out.far_z  = out.near_z + 1.0f / p.m[10];
    return out;
}

inline Mat4f inverse_perspective_mat(const Mat4f& p)
{
    Mat4f r{};
    r.m[0]  = 1.0f / p.m[0];
    r.m[5]  = 1.0f / p.m[5];
    r.m[11] = 1.0f / p.m[14];
    r.m[14] = 1.0f;
    r.m[15] = -p.m[10] / p.m[14];
    return r;
}

inline Mat4f inverse_ortho_lh_mat(const Mat4f& p)
{
    Mat4f r{};
    r.m[0]  = 1.0f / p.m[0];
    r.m[5]  = 1.0f / p.m[5];
    r.m[10] = 1.0f / p.m[10];
    r.m[12] = -p.m[12] / p.m[0];
    r.m[13] = -p.m[13] / p.m[5];
    r.m[14] = -p.m[14] / p.m[10];
    r.m[15] = 1.0f;
    return r;
}

} // namespace Ignis::Math
