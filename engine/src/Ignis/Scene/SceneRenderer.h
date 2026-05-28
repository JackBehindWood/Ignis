#pragma once

#include "Scene.h"
#include "Ignis/Scene/CameraData.h"
#include "Ignis/Asset/AssetManager.h"
#include "Ignis/Math/Math.h"
#include "Ignis/Rendering/GRI/GRICommandList.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"
#include "Ignis/Rendering/RenderMesh.h"
#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/RenderGraph/RGResource.h"
#include "Ignis/Rendering/Renderer.h"

namespace Ignis
{
class RGBuilder;

struct CullProxy
{
    Math::Vec3f world_center;
    float       world_radius;
    uint32_t    entity_index;
};

struct GPUInstanceData
{
    Math::Mat4f world_matrix;
    uint32_t    material_index;
    uint32_t    padding[3];
};

struct RenderCommand
{
    uint64_t          sort_key;
    const RenderMesh* mesh;
    const Material*   material;
    GRIPipelineState* pso;
    uint32_t          instance_count;
    uint32_t          base_instance;
};

class SceneRenderer
{
public:
    void prepare(Scene& scene);

    void render_scene(Scene& scene, const CameraData& camera, RGBuilder& builder, RGTextureHandle backbuffer,
                      AssetID scene_texture_id);

private:
    struct ScenePassParams
    {
        GRITexture2D* scene_texture;
    };

    struct VisibleItem
    {
        const RenderMesh* mesh;
        const Material*   material;
        GRIPipelineState* depth_pso;
        Math::Mat4f       world;
        uint64_t          depth_key;
        uint64_t          fwd_key;
    };

    static GRITexture2D* resolve_texture(AssetID id);
    void                 build_cull_proxies(Scene& scene, const Math::Mat4f& cam_view);
    void                 build_commands();

    static constexpr uint32_t k_max_instances        = 4096;
    static constexpr float    k_depth_range          = 1000.0f;
    static constexpr uint32_t k_instance_buffer_slot = 28;

    Vector<CullProxy>       m_cull_proxies;
    Vector<VisibleItem>     m_pool;
    Vector<VisibleItem>     m_visible;
    Vector<RenderCommand>   m_depth_cmds;
    Vector<RenderCommand>   m_fwd_cmds;
    Vector<GPUInstanceData> m_instance_data;
    GRIBufferPtr            m_instance_buffer;
};

} // namespace Ignis
