#pragma once

#if !defined(INTERNAL_DECORATOR)
	#define INTERNAL_DECORATOR(method) cmd_list.get_context().method
#endif

namespace Ignis
{
	void GRICommandBeginDrawingViewport::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(begin_drawing_viewport)(viewport, render_target);
	}

	void GRICommandBeginFrame::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(begin_frame)();
	}

	void GRICommandEndFrame::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(end_frame)();
	}

	void GRICommandBeginRenderPass::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(begin_render_pass)(info);
	}

	void GRICommandEndRenderPass::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(end_render_pass)();
	}

	void GRICommandSetVertexBuffer::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(set_vertex_buffer)(buffer, offset, buffer_index);
	}

	void GRICommandSetGraphicsPipelineState::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(set_graphics_pipeline_state)(pipeline_state);
	}

	void GRICommandDrawPrimitive::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(draw_primitives)(vertex_count, first_vertex);
	}

	void GRICommandSetIndexBuffer::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(set_index_buffer)(buffer, format, offset);
	}

	void GRICommandDrawIndexedPrimitive::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(draw_indexed_primitives)(index_count, first_index, vertex_offset);
	}

	void GRICommandSetUniformBuffer::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(set_uniform_buffer)(buffer, slot, stage, offset);
	}
}

#undef INTERNAL_DECORATOR