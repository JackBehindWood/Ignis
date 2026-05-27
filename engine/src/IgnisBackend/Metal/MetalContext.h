#pragma once

#include <Ignis/Rendering/GRI/GRIContext.h>
#include "MetalDevice.h"
#include "MetalStateCache.h"
#include "MetalCommandEncoder.h"

#include <Metal/Metal.hpp>

namespace Ignis
{
class MetalCommandBuffer;

class MetalCommandContext : public GRICommandContext
{
private:
    MetalDevice&               m_device;
    MetalStateCache            m_state_cache;
    MetalCommandBuffer*        m_command_buffer;
    MetalViewport*             m_active_viewport;
    MetalRenderCommandEncoder* m_render_encoder;
    MTL::SamplerState*         m_default_sampler;

public:
    MetalCommandContext(MetalDevice& device);
    virtual ~MetalCommandContext() = default;

    virtual void begin_frame() override;
    virtual void end_frame() override;

    virtual void begin_drawing_viewport(GRIViewport* viewport, GRITexture2D* render_target) override;

    virtual void set_render_targets(uint32_t num_render_targets, const GRIRenderTargetView* render_targets,
                                    const GRIDepthRenderTargetView* depth_stencil_target) override;
    virtual void set_render_targets_and_clear(const GRIRenderTargetsInfo& info) override;

    virtual void begin_render_pass(const GRIRenderPassInfo& info) override;
    virtual void end_render_pass() override;

    virtual void set_vertex_buffer(GRIBuffer* buffer, uint32_t offset, uint32_t buffer_index) override;
    virtual void set_index_buffer(GRIBuffer* buffer, GRIIndexFormat format, uint32_t offset) override;
    virtual void set_uniform_buffer(GRIBuffer* buffer, uint32_t slot, GRIShaderStage stage, uint32_t offset) override;

    virtual void set_texture(GRITexture2D* texture, uint32_t slot, GRIShaderStage stage) override;

    virtual void set_graphics_pipeline_state(GRIPipelineState* pipeline_state) override;
    virtual void draw_primitives(uint32_t vertex_count, uint32_t first_vertex) override;
    virtual void draw_indexed_primitives(uint32_t index_count, uint32_t first_index, int32_t vertex_offset) override;
};
} // namespace Ignis
