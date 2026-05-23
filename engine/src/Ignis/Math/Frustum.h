#pragma once

#include "Vec3.h"
#include "Vec4.h"
#include "Mat4.h"

namespace Ignis::Math
{
    struct Frustum
    {
        Vec4f planes[6]; // left, right, bottom, top, near, far — normalized
    };

    // Extract frustum planes from a clip matrix (P*V), right-handed, Metal [0,1] depth range.
    inline Frustum extract_frustum(const Mat4f& clip)
    {
        const Vec4f r0 = clip.row(0);
        const Vec4f r1 = clip.row(1);
        const Vec4f r2 = clip.row(2);
        const Vec4f r3 = clip.row(3);

        Frustum f;
        f.planes[0] = normalized(r3 + r0); // left
        f.planes[1] = normalized(r3 - r0); // right
        f.planes[2] = normalized(r3 + r1); // bottom
        f.planes[3] = normalized(r3 - r1); // top
        f.planes[4] = normalized(r2);      // near  (Metal [0,1]: z/w >= 0)
        f.planes[5] = normalized(r3 - r2); // far   (Metal [0,1]: z/w <= 1)
        return f;
    }

    // Returns false if the sphere is entirely outside any frustum half-space (safe to cull).
    inline bool frustum_contains_sphere(const Frustum& f, Vec3f center, float radius)
    {
        for (const Vec4f& plane : f.planes)
        {
            if (plane.x * center.x + plane.y * center.y + plane.z * center.z + plane.w < -radius)
                return false;
        }
        return true;
    }

} // namespace Ignis::Math
