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

struct SceneRenderHandles
{
    RGTextureHandle color;
    RGTextureHandle depth;
};

struct CullProxy
{
    Math::Vec3f world_center;
    float       world_radius;
    uint32_t    entity_index;
};

// Mirrors MTLDrawIndexedPrimitivesIndirectArguments / VkDrawIndexedIndirectCommand exactly.
// Phase 6: upload array to GPU buffer, replace emit loop with a single indirect draw call.
struct DrawIndexedArguments
{
    uint32_t index_count;
    uint32_t instance_count;
    uint32_t first_index;
    int32_t  base_vertex;
    uint32_t base_instance;
};

struct DrawBatch
{
    DrawIndexedArguments args;
    const RenderMesh*    mesh;
    GRIPipelineState*    pso;
    const Material*      material;
    uint16_t             buffer_id;
    uint16_t             pso_id;
    uint16_t             material_id;
};

struct GPUInstanceData
{
    Math::Mat4f world_matrix;
    uint32_t    material_index;
    uint32_t    padding[3];
};

class SceneRenderer
{
public:
    void prepare(Scene& scene);

    SceneRenderHandles render_scene(Scene&, const CameraData&, RGBuilder&);

    void resize(uint32_t w, uint32_t h);

    GRITexture2D* get_color_rt() const
    {
        return m_color_rt.get();
    }
    GRITexture2D* get_depth_rt() const
    {
        return m_depth_rt.get();
    }

private:
    struct VisibleItem
    {
        Math::Mat4f       world_matrix;
        const RenderMesh* mesh;
        GRIPipelineState* pso;
        GRIPipelineState* depth_pso;
        const Material*   material;
        uint16_t          buffer_id;
        uint16_t          pso_id;
        uint16_t          depth_pso_id;
        uint16_t          material_id;
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
    Vector<DrawBatch>       m_depth_batches;
    Vector<DrawBatch>       m_fwd_batches;
    Vector<GPUInstanceData> m_instance_data;
    GRIBufferPtr            m_instance_buffer;

    GRITexture2DPtr m_color_rt;
    GRITexture2DPtr m_depth_rt;
    uint32_t        m_rt_width  = 0;
    uint32_t        m_rt_height = 0;

    SharedPtr<Material> m_fallback_material;
    uint16_t            m_fallback_material_id = 0xFFFFu;
};

} // namespace Ignis
