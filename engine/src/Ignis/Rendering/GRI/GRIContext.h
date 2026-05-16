#pragma once

#include "GRIResource.h"

namespace Ignis
{
    class GRICommandContext
    {
    public:
        virtual ~GRICommandContext() = default;

        virtual void begin_frame() = 0;
        virtual void end_frame() = 0;

        virtual void begin_drawing_viewport(GRIViewport* viewport, GRITexture2D* render_target) = 0;

		virtual void set_render_targets(uint32_t num_render_targets, const GRIRenderTargetView* render_targets, const GRIDepthRenderTargetView* depth_stencil_target) = 0;
        virtual void set_render_targets_and_clear(const GRIRenderTargetsInfo& info) = 0;

        virtual void begin_render_pass(const GRIRenderPassInfo& info) = 0;
        virtual void end_render_pass() = 0;

        virtual void set_vertex_buffer(GRIBuffer* buffer, uint32_t offset, uint32_t buffer_index) = 0;
        virtual void set_index_buffer(GRIBuffer* buffer, GRIIndexFormat format, uint32_t offset) = 0;
        virtual void set_uniform_buffer(GRIBuffer* buffer, uint32_t slot, GRIShaderStage stage, uint32_t offset = 0) = 0;

        virtual void set_graphics_pipeline_state(GRIPipelineState* pipeline_state) = 0;
        virtual void draw_primitives(uint32_t vertex_count, uint32_t first_vertex) = 0;
        virtual void draw_indexed_primitives(uint32_t index_count, uint32_t first_index, int32_t vertex_offset) = 0;
    };
} // namespace Ignis
