#pragma once

#include "RenderScene.h"
#include "Ignis/Rendering/GRI/GRICommandList.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"
#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/RenderGraph/RGResource.h"

namespace Ignis
{
class RGBuilder;

struct SceneRenderHandles
{
    RGTextureHandle color;
    RGTextureHandle depth;
};

struct DrawBatch
{
    DrawIndexedArguments args;
    const RenderMesh*    mesh;
    GRIPipelineState*    pso;
    const Material*      material;
    GRITexture2D*        texture = nullptr;
    uint16_t             buffer_id;
    uint16_t             pso_id;
    uint16_t             material_id;
};

class SceneRenderer
{
public:
    SceneRenderHandles render_scene(const RenderScene&, RGBuilder&);
    RGTextureHandle    render_tonemap(RGTextureHandle hdr_color, RGBuilder&);

    void resize(uint32_t w, uint32_t h);

    GRITexture2D* get_color_rt() const
    {
        return m_color_rt.get();
    }
    GRITexture2D* get_ldr_rt() const
    {
        return m_ldr_rt.get();
    }
    GRITexture2D* get_depth_rt() const
    {
        return m_depth_rt.get();
    }

private:
    void build_commands(const RenderScene&);
    void ensure_gpu_buffers();

    static constexpr uint32_t k_entity_buffer_slot   = static_cast<uint32_t>(DefaultBindings::InstanceData);
    static constexpr uint32_t k_instance_buffer_slot = static_cast<uint32_t>(DefaultBindings::VisibleIndices);

    Vector<DrawBatch> m_depth_batches;
    Vector<DrawBatch> m_fwd_batches;

    // Persistent GPU buffers — sized by engine constants in RenderScene.h.
    GRIBufferPtr m_cull_input_buffer;       // GPUCullInstance[k_max_instances]
    GRIBufferPtr m_visible_indices_buffer;  // uint32_t[k_max_instances*2]
    GRIBufferPtr m_gpu_cull_visible_buffer; // uint32_t[k_max_instances*2] — GPU cull scratch
    GRIBufferPtr m_atomic_counter_buffer;   // uint32_t[1]
    GRIBufferPtr m_draw_args_buffer;        // DrawIndexedArguments[k_max_batches]
    GRIBufferPtr m_cull_cb;                 // GPUCullConstants
    GRIBufferPtr m_entity_data_buffer;      // GPUInstanceData[k_max_instances]

    GRITexture2DPtr m_color_rt;
    GRITexture2DPtr m_ldr_rt;
    GRITexture2DPtr m_depth_rt;
    uint32_t        m_rt_width  = 0;
    uint32_t        m_rt_height = 0;
};

} // namespace Ignis
