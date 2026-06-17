#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Math/Math.h"

#include "IDComponent.h"
#include "NameComponent.h"
#include "CameraComponent.h"
#include "ScriptComponent.h"

namespace Ignis
{

IG_CLASS(Component)
struct TransformComponent
{
    IG_PROPERTY(EditAnywhere, SaveGame)
    Math::Vec3f position = Math::Vec3f::zero();

    IG_PROPERTY(EditAnywhere, SaveGame)
    Math::Quatf rotation = Math::Quatf::identity();

    IG_PROPERTY(EditAnywhere, SaveGame)
    Math::Vec3f scale = Math::Vec3f::one();

    Math::Mat4f to_mat4() const
    {
        return Math::translate(position) * rotation.to_mat4() * Math::scale(scale);
    }
};

enum class MeshCategory : uint8_t
{
    Static,
    Dynamic,
    Skinned,
};

IG_CLASS(Component)
struct MeshRendererComponent
{
    IG_PROPERTY(EditAnywhere, SaveGame)
    AssetID mesh_id;

    IG_PROPERTY(EditAnywhere, SaveGame)
    MeshCategory category = MeshCategory::Static;

    IG_PROPERTY(EditAnywhere, SaveGame)
    bool is_visible = true;
};

IG_CLASS(Component)
struct MaterialComponent
{
    IG_PROPERTY(EditAnywhere, SaveGame)
    AssetID material_id;

    IG_PROPERTY(EditAnywhere, SaveGame)
    Math::LinearColour albedo_colour = {1.0f, 1.0f, 1.0f, 1.0f};
    IG_PROPERTY(EditAnywhere, SaveGame)
    Math::LinearColour emissive_colour = {0.0f, 0.0f, 0.0f, 0.0f};
    IG_PROPERTY(EditAnywhere, SaveGame)
    float alpha_cutoff = 0.0f;
    IG_PROPERTY(EditAnywhere, SaveGame)
    float emissive_intensity = 1.0f;

    IG_PROPERTY(EditAnywhere, SaveGame)
    AssetID albedo_tex;
    IG_PROPERTY(EditAnywhere, SaveGame)
    AssetID normal_tex;
    IG_PROPERTY(EditAnywhere, SaveGame)
    AssetID roughness_tex;
    IG_PROPERTY(EditAnywhere, SaveGame)
    AssetID metallic_tex;
    IG_PROPERTY(EditAnywhere, SaveGame)
    AssetID ao_tex;
    IG_PROPERTY(EditAnywhere, SaveGame)
    AssetID emissive_tex;

    bool params_dirty = true;
};

IG_CLASS(Component)
struct TextureComponent
{
    IG_PROPERTY(EditAnywhere, SaveGame)
    AssetID texture_id;
};

IG_CLASS(Component)
struct DirectionalLightComponent
{
    IG_PROPERTY(EditAnywhere, SaveGame)
    Math::Vec3f color = Math::Vec3f(1.0f, 1.0f, 1.0f);

    IG_PROPERTY(EditAnywhere, SaveGame)
    float intensity = 1.0f;

    IG_PROPERTY(EditAnywhere, SaveGame)
    Math::Vec3f direction = Math::Vec3f(0.0f, -1.0f, 0.0f);
};

IG_CLASS(Component)
struct PointLightComponent
{
    IG_PROPERTY(EditAnywhere, SaveGame)
    Math::Vec3f color = Math::Vec3f(1.0f, 1.0f, 1.0f);

    IG_PROPERTY(EditAnywhere, SaveGame)
    float intensity = 1.0f;

    IG_PROPERTY(EditAnywhere, SaveGame)
    float radius = 10.0f;
};

IG_CLASS(Component)
struct SpotLightComponent
{
    IG_PROPERTY(EditAnywhere, SaveGame)
    Math::Vec3f color = Math::Vec3f(1.0f, 1.0f, 1.0f);

    IG_PROPERTY(EditAnywhere, SaveGame)
    float intensity = 1.0f;

    IG_PROPERTY(EditAnywhere, SaveGame)
    float radius = 10.0f;

    IG_PROPERTY(EditAnywhere, SaveGame)
    Math::Vec3f direction = Math::Vec3f(0.0f, -1.0f, 0.0f);

    IG_PROPERTY(EditAnywhere, SaveGame)
    float inner_cone_angle = 20.0f;

    IG_PROPERTY(EditAnywhere, SaveGame)
    float outer_cone_angle = 30.0f;
};

} // namespace Ignis
