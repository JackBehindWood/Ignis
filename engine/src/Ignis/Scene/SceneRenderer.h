#pragma once

#include "Scene.h"
#include "Ignis/Scene/CameraData.h"
#include "Ignis/Asset/AssetManager.h"
#include "Ignis/Math/Math.h"
#include "Ignis/Rendering/GRI/GRICommandList.h"
#include "Ignis/Rendering/RenderMesh.h"
#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/RenderGraph/RGResource.h"

// Temporary scene renderer — no frustum culling, sorting, or batching.
namespace Ignis
{
class RGBuilder;

struct FrameDrawItem
{
    const RenderMesh* mesh     = nullptr;
    const Material*   material = nullptr;
    Math::Mat4f       world    = Math::Mat4f::identity();
    float             depth    = 0.0f;
};

class SceneRenderer
{
public:
    // TODO: add a render_scene cersion without CameraData, here we should get the active casmera's CameraData;
    void render_scene(Scene& scene, const CameraData& camera, RGBuilder& builder, RGTextureHandle backbuffer,
                      AssetID scene_texture_id);

private:
    struct ScenePassParams
    {
        GRITexture2D* scene_texture;
        Math::Mat4f   view_projection;
    };

    static GRITexture2D* resolve_texture(AssetID id);

    // Persistent across frames; .clear()'d at the top of each render_scene call.
    Vector<FrameDrawItem> m_opaque;
    Vector<FrameDrawItem> m_transparent;
};

} // namespace Ignis
