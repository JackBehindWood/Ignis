#pragma once

#include "Scene.h"
#include "Ignis/Scene/CameraData.h"
#include "Ignis/Asset/AssetManager.h"
#include "Ignis/Math/Math.h"
#include "Ignis/Rendering/GRI/GRICommandList.h"
#include "Ignis/Rendering/RenderMesh.h"
#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/RenderGraph/RGResource.h"
#include "Ignis/Rendering/Renderer.h"

namespace Ignis
{
class RGBuilder;

struct FrameDrawItem
{
    const RenderMesh* mesh      = nullptr;
    const Material*   material  = nullptr; // forward pass material (depth_write=false)
    GRIPipelineState* depth_pso = nullptr; // depth-only PSO for pre-pass
    Math::Mat4f       world     = Math::Mat4f::identity();
    float             depth     = 0.0f;
};

// TODO: we should probably create a render shader for the depth pass and we should add/improve sorting, culling and
// batching (Some of these might be moved to the Rendering Module and not be in the SceneRenderer!).
class SceneRenderer
{
public:
    // Upload GPU resources for all scene meshes. Call once after assets are loaded.
    void prepare(Scene& scene);

    void render_scene(Scene& scene, const CameraData& camera, RGBuilder& builder, RGTextureHandle backbuffer,
                      AssetID scene_texture_id);

private:
    struct ScenePassParams
    {
        GRITexture2D* scene_texture;
    };

    static GRITexture2D* resolve_texture(AssetID id);

    Vector<FrameDrawItem> m_opaque;
    Vector<FrameDrawItem> m_transparent;
};

} // namespace Ignis
