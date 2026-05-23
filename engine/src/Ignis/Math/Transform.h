#pragma once

#include "Vec3.h"
#include "Quat.h"
#include "Mat4.h"
#include "MathUtils.h"

namespace Ignis::Math
{
    // Decomposed world transform: position + rotation (unit quaternion) + scale.
    // to_mat4() composes T * R * S.
    template<typename T>
    struct Transform
    {
        Vec3<T> position = Vec3<T>::zero();
        Quat<T> rotation = Quat<T>::identity();
        Vec3<T> scale    = Vec3<T>::one();

        Mat4<T> to_mat4() const
        {
            return translate(position) * rotation.to_mat4() * Ignis::Math::scale(scale);
        }

        // Decompose a column-major TRS matrix into position, scale, and rotation.
        // Assumes the matrix was composed as T * R * S with no shear.
        static Transform decompose(const Mat4<T>& mat)
        {
            Transform t;

            t.position = { mat.m[12], mat.m[13], mat.m[14] };

            Vec3<T> col0{ mat.m[0], mat.m[1], mat.m[2] };
            Vec3<T> col1{ mat.m[4], mat.m[5], mat.m[6] };
            Vec3<T> col2{ mat.m[8], mat.m[9], mat.m[10] };

            t.scale.x = col0.length();
            t.scale.y = col1.length();
            t.scale.z = col2.length();

            // Guard against zero-scale columns
            if (t.scale.x > T(0)) col0 /= t.scale.x;
            if (t.scale.y > T(0)) col1 /= t.scale.y;
            if (t.scale.z > T(0)) col2 /= t.scale.z;

            // Extract quaternion from normalized rotation matrix (column-major)
            // Uses Shepperd's method
            T trace = col0.x + col1.y + col2.z;
            if (trace > T(0))
            {
                T s = T(0.5) / Math::sqrt(trace + T(1));
                t.rotation.w = T(0.25) / s;
                t.rotation.x = (col1.z - col2.y) * s;
                t.rotation.y = (col2.x - col0.z) * s;
                t.rotation.z = (col0.y - col1.x) * s;
            }
            else if (col0.x > col1.y && col0.x > col2.z)
            {
                T s = T(2) * Math::sqrt(T(1) + col0.x - col1.y - col2.z);
                t.rotation.w = (col1.z - col2.y) / s;
                t.rotation.x = T(0.25) * s;
                t.rotation.y = (col1.x + col0.y) / s;
                t.rotation.z = (col2.x + col0.z) / s;
            }
            else if (col1.y > col2.z)
            {
                T s = T(2) * Math::sqrt(T(1) + col1.y - col0.x - col2.z);
                t.rotation.w = (col2.x - col0.z) / s;
                t.rotation.x = (col1.x + col0.y) / s;
                t.rotation.y = T(0.25) * s;
                t.rotation.z = (col2.y + col1.z) / s;
            }
            else
            {
                T s = T(2) * Math::sqrt(T(1) + col2.z - col0.x - col1.y);
                t.rotation.w = (col0.y - col1.x) / s;
                t.rotation.x = (col2.x + col0.z) / s;
                t.rotation.y = (col2.y + col1.z) / s;
                t.rotation.z = T(0.25) * s;
            }

            return t;
        }

        static constexpr Transform identity() { return {}; }
    };

    using Transformf = Transform<float>;
    using Transformd = Transform<double>;
} // namespace Ignis::Math
