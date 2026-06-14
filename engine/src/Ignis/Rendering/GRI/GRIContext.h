#pragma once

#include "GRIResource.h"
#include <Ignis/Foundation/Vector.h>

namespace Ignis
{
class GRICommandContext
{
public:
    virtual ~GRICommandContext() = default;

    virtual void begin_frame() = 0;
    virtual void end_frame()   = 0;

    virtual void begin_drawing_viewport(GRIViewport* viewport, GRITexture2D* render_target) = 0;

    virtual void set_render_targets(uint32_t num_render_targets, const GRIRenderTargetView* render_targets,
                                    const GRIDepthRenderTargetView* depth_stencil_target) = 0;
    virtual void set_render_targets_and_clear(const GRIRenderTargetsInfo& info)           = 0;

    virtual void begin_render_pass(const GRIRenderPassInfo& info, const Vector<GRIBuffer*>& storage_buffers,
                                   const Vector<GRITexture2D*>& storage_textures) = 0;
    virtual void end_render_pass()                                                = 0;

    virtual void begin_compute_pass(const Vector<GRIBuffer*>&    storage_bufs,
                                    const Vector<GRITexture2D*>& storage_textures) = 0;
    virtual void end_compute_pass()                                                = 0;

    virtual void set_vertex_buffer(GRIBuffer* buffer, uint32_t offset, uint32_t buffer_index)                    = 0;
    virtual void set_index_buffer(GRIBuffer* buffer, GRIIndexFormat format, uint32_t offset)                     = 0;
    virtual void set_uniform_buffer(GRIBuffer* buffer, uint32_t slot, GRIShaderStage stage, uint32_t offset = 0) = 0;

    virtual void set_texture(GRITexture2D* texture, uint32_t slot, GRIShaderStage stage) = 0;

    virtual void set_graphics_pipeline_state(GRIPipelineState* pipeline_state)                              = 0;
    virtual void draw_primitives(uint32_t vertex_count, uint32_t first_vertex)                              = 0;
    virtual void draw_indexed_primitives(uint32_t index_count, uint32_t first_index, int32_t vertex_offset) = 0;
    virtual void draw_indexed_primitives_instanced(uint32_t index_count, uint32_t instance_count,
                                                   uint32_t base_instance, uint32_t first_index,
                                                   int32_t vertex_offset)                                   = 0;

    virtual void set_compute_pipeline_state(GRIComputePipelineState* pso)                                    = 0;
    virtual void set_storage_buffer(GRIBuffer* buffer, uint32_t slot)                                        = 0;
    virtual void set_storage_texture(GRITexture2D* texture, uint32_t slot, uint32_t mip_level = 0,
                                     uint32_t array_slice = 0)                                               = 0;
    virtual void set_compute_sampler(GRISamplerState* sampler, uint32_t slot)                                = 0;
    virtual void dispatch(uint32_t x, uint32_t y, uint32_t z)                                                = 0;
    virtual void draw_indexed_primitives_indirect(GRIBuffer* args_buf, uint32_t byte_offset)                 = 0;
    virtual void memory_barrier(GRIResource* resource, GRIAccessFlags old_access, GRIAccessFlags new_access) = 0;
};
} // namespace Ignis
