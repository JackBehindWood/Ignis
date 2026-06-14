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
		INTERNAL_DECORATOR(begin_render_pass)(info, storage_buffers, storage_textures);
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

	void GRICommandDrawIndexedPrimitiveInstanced::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(draw_indexed_primitives_instanced)(index_count, instance_count, base_instance,
		                                                      first_index, vertex_offset);
	}

	void GRICommandSetUniformBuffer::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(set_uniform_buffer)(buffer, slot, stage, offset);
	}

	void GRICommandSetTexture::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(set_texture)(texture, slot, stage);
	}

	void GRICommandSetComputePipelineState::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(set_compute_pipeline_state)(pso);
	}

	void GRICommandSetStorageBuffer::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(set_storage_buffer)(buffer, slot);
	}

	void GRICommandSetStorageTexture::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(set_storage_texture)(texture, slot, mip_level, array_slice);
	}

	void GRICommandSetComputeSampler::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(set_compute_sampler)(sampler, slot);
	}

	void GRICommandDispatch::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(dispatch)(x, y, z);
	}

	void GRICommandDrawIndexedPrimitivesIndirect::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(draw_indexed_primitives_indirect)(args_buf, byte_offset);
	}

	void GRICommandMemoryBarrier::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(memory_barrier)(resource, old_access, new_access);
	}

	void GRICommandBeginComputePass::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(begin_compute_pass)(storage_buffers, storage_textures);
	}

	void GRICommandEndComputePass::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(end_compute_pass)();
	}
}

#undef INTERNAL_DECORATOR