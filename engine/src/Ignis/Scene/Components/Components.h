#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Math/Math.h"

#include "IDComponent.h"
#include "NameComponent.h"
#include "CameraComponent.h"
#include "ScriptComponent.h"

namespace Ignis
{

// TODO: we want to use a left handed coordiante system!!!!!!!!!

// TODO: We should maybe only use Math::Transformf and add a convert struct to YAMLMathCodecs.h to get the individual
// components and serialise these! But it also depends on the Scene Renderer!
IG_CLASS(Component)
struct TransformComponent
{
    IG_PROPERTY(position)
    Math::Vec3f position = Math::Vec3f::zero();

    IG_PROPERTY(rotation)
    Math::Quatf rotation = Math::Quatf::identity();

    IG_PROPERTY(scale)
    Math::Vec3f scale = Math::Vec3f::one();

    Math::Mat4f to_mat4() const
    {
        return Math::translate(position) * rotation.to_mat4() * Math::scale(scale);
    }
};

// TODO: split into separate MeshRendererComponent and MaterialComponent!
IG_CLASS(Component)
struct MeshComponent
{
    IG_PROPERTY(mesh_id)
    AssetID mesh_id;

    IG_PROPERTY(material_id)
    AssetID material_id;

    IG_PROPERTY(is_visible)
    bool is_visible = true;
};

} // namespace Ignis
