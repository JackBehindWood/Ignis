#pragma once

#include <Ignis/Rendering/GRI/GRIContext.h>
#include "MetalDevice.h"
#include "MetalStateCache.h"
#include "MetalCommandEncoder.h"
#include "MetalCommandBuffer.h"
#include "MetalBindlessArray.h"

#include <Metal/Metal.hpp>

namespace Ignis
{
class MetalCommandContext : public GRICommandContext
{
private:
    MetalDevice&                m_device;
    MetalStateCache             m_state_cache;
    MetalCommandBuffer*         m_command_buffer;
    MetalCommandBuffer*         m_compute_command_buffer;
    MetalViewport*              m_active_viewport;
    MetalRenderCommandEncoder*  m_render_encoder;
    MetalComputeCommandEncoder* m_compute_encoder;
    MTL::Size                   m_active_threadgroup_size;
    MTL::SamplerState*          m_default_sampler;
    MetalBindlessArray          m_bindless_array;

    void end_render_encoder()
    {
        if (!m_render_encoder)
        {
            return;
        }
        delete m_render_encoder;
        m_render_encoder = nullptr;
    }

    void end_compute_encoder()
    {
        if (!m_compute_encoder)
        {
            return;
        }
        delete m_compute_encoder;
        m_compute_encoder = nullptr;
    }

public:
    MetalCommandContext(MetalDevice& device);
    virtual ~MetalCommandContext() = default;

    virtual void begin_frame() override;
    virtual void end_frame() override;

    virtual void begin_drawing_viewport(GRIViewport* viewport, GRITexture2D* render_target) override;

    virtual void set_render_targets(uint32_t num_render_targets, const GRIRenderTargetView* render_targets,
                                    const GRIDepthRenderTargetView* depth_stencil_target) override;
    virtual void set_render_targets_and_clear(const GRIRenderTargetsInfo& info) override;

    virtual void begin_render_pass(const GRIRenderPassInfo& info, const Vector<GRIBuffer*>& storage_buffers,
                                   const Vector<GRITexture2D*>& storage_textures) override;
    virtual void end_render_pass() override;

    virtual void begin_compute_pass(const Vector<GRIBuffer*>&    storage_bufs,
                                    const Vector<GRITexture2D*>& storage_textures) override;
    virtual void end_compute_pass() override;

    virtual void set_vertex_buffer(GRIBuffer* buffer, uint32_t offset, uint32_t buffer_index) override;
    virtual void set_index_buffer(GRIBuffer* buffer, GRIIndexFormat format, uint32_t offset) override;
    virtual void set_uniform_buffer(GRIBuffer* buffer, uint32_t slot, GRIShaderStage stage, uint32_t offset) override;

    virtual void set_texture(GRITexture2D* texture, uint32_t slot, GRIShaderStage stage) override;

    virtual void set_graphics_pipeline_state(GRIPipelineState* pipeline_state) override;
    virtual void draw_primitives(uint32_t vertex_count, uint32_t first_vertex) override;
    virtual void draw_indexed_primitives(uint32_t index_count, uint32_t first_index, int32_t vertex_offset) override;
    virtual void draw_indexed_primitives_instanced(uint32_t index_count, uint32_t instance_count,
                                                   uint32_t base_instance, uint32_t first_index,
                                                   int32_t vertex_offset) override;

    virtual void set_compute_pipeline_state(GRIComputePipelineState* pso) override;
    virtual void set_storage_buffer(GRIBuffer* buffer, uint32_t slot) override;
    virtual void set_storage_texture(GRITexture2D* texture, uint32_t slot, uint32_t mip_level = 0,
                                     uint32_t array_slice = 0) override;
    virtual void set_compute_sampler(GRISamplerState* sampler, uint32_t slot) override;
    virtual void dispatch(uint32_t x, uint32_t y, uint32_t z) override;
    virtual void draw_indexed_primitives_indirect(GRIBuffer* args_buf, uint32_t byte_offset) override;
    virtual void memory_barrier(GRIResource* resource, GRIAccessFlags old_access, GRIAccessFlags new_access) override;

    void     init_bindless_array(MTL::Function* ps_function);
    uint32_t register_bindless_texture(GRITexture2DPtr texture, GRISamplerStatePtr sampler);

    inline MTL::CommandBuffer* get_current_command_buffer() const
    {
        return m_command_buffer ? m_command_buffer->get_buffer() : nullptr;
    }
    inline MetalViewport* get_active_viewport() const
    {
        return m_active_viewport;
    }
};
} // namespace Ignis
