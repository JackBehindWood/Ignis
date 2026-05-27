#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Math/Math.h"

namespace Ignis
{

struct TransformComponent
{
    Math::Transformf transform;
};

// TODO: split into separate MeshRendererComponent and MaterialComponent!
struct MeshComponent
{
    AssetID mesh_id;
    AssetID material_id;
    bool    is_visible = true;
};

struct CameraComponent
{
    Math::Mat4f view       = Math::Mat4f::identity();
    Math::Mat4f projection = Math::Mat4f::identity();
};

} // namespace Ignis
