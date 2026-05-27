#pragma once

#include "Ignis/Math/Math.h"

namespace Ignis
{

struct CameraData
{
    Math::Mat4f   view;
    Math::Mat4f   projection;
    Math::Mat4f   view_projection;
    Math::Vec3f   position;
    Math::Frustum frustum;

    static CameraData identity()
    {
        CameraData d;
        d.view            = Math::Mat4f::identity();
        d.projection      = Math::Mat4f::identity();
        d.view_projection = Math::Mat4f::identity();
        d.position        = Math::Vec3f(0.0f, 0.0f, 0.0f);
        return d;
    }

    static CameraData from_matrices(const Math::Mat4f& view, const Math::Mat4f& proj, const Math::Vec3f& pos)
    {
        CameraData d;
        d.view            = view;
        d.projection      = proj;
        d.view_projection = proj * view;
        d.position        = pos;
        d.frustum         = Math::extract_frustum(d.view_projection);
        return d;
    }
};

} // namespace Ignis
