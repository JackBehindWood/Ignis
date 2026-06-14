#include "igpch.h"
#include "MetalContext.h"
#include "MetalCommandBuffer.h"
#include "MetalResource.h"

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

namespace Ignis
{
MetalCommandContext::MetalCommandContext(MetalDevice& device)
    : m_device(device),
      m_state_cache(device),
      m_command_buffer(nullptr),
      m_compute_command_buffer(nullptr),
      m_active_viewport(nullptr),
      m_render_encoder(nullptr),
      m_compute_encoder(nullptr),
      m_active_threadgroup_size{1, 1, 1},
      m_default_sampler(nullptr)
{
    MTL::SamplerDescriptor* sd = MTL::SamplerDescriptor::alloc()->init();
    sd->setMinFilter(MTL::SamplerMinMagFilterLinear);
    sd->setMagFilter(MTL::SamplerMinMagFilterLinear);
    sd->setSAddressMode(MTL::SamplerAddressModeRepeat);
    sd->setTAddressMode(MTL::SamplerAddressModeRepeat);
    m_default_sampler = device.get_device()->newSamplerState(sd);
    sd->release();
}

void MetalCommandContext::begin_frame()
{
    IG_CORE_ASSERT(!m_command_buffer, "begin_frame called while a frame is already active (end_frame never called)");

    m_command_buffer         = new MetalCommandBuffer(m_device.graphics_queue().get_queue()->commandBuffer());
    m_compute_command_buffer = new MetalCommandBuffer(m_device.compute_queue().get_queue()->commandBuffer());
}

void MetalCommandContext::end_frame()
{
    IG_CORE_ASSERT(m_command_buffer, "end_frame called before begin_frame");
    IG_CORE_ASSERT(!m_render_encoder, "end_frame called with an open render pass — call end_render_pass first");
    IG_CORE_ASSERT(
        !m_compute_encoder,
        "end_frame called with an open compute pass — dispatch must be followed by a transition or memory_barrier");

    if (m_active_viewport)
    {
        m_command_buffer->get_buffer()->presentDrawable(
            reinterpret_cast<const MTL::Drawable*>(m_active_viewport->get_drawable()));
        m_active_viewport->release_drawable();
        m_active_viewport = nullptr;
    }

    m_compute_command_buffer->get_buffer()->commit();
    delete m_compute_command_buffer;
    m_compute_command_buffer = nullptr;

    m_command_buffer->get_buffer()->commit();
    delete m_command_buffer;
    m_command_buffer = nullptr;
}

void MetalCommandContext::begin_render_pass(const GRIRenderPassInfo& info, const Vector<GRIBuffer*>& storage_buffers,
                                            const Vector<GRITexture2D*>& storage_textures)
{
    IG_CORE_ASSERT(!m_render_encoder, "begin_render_pass called with a render pass already open");
    IG_CORE_ASSERT(m_command_buffer, "begin_render_pass called before begin_frame");
    end_compute_encoder();

    GRIRenderPassInfo merged         = m_state_cache.get_pending_pass_info();
    const int32_t     pending_count  = merged.get_num_colour_targets();
    const int32_t     explicit_count = static_cast<int32_t>(info.num_explicit_colour_targets);
    const int32_t     num_slots      = pending_count > explicit_count ? pending_count : explicit_count;

    for (int32_t i = 0; i < num_slots; i++)
    {
        if (i < explicit_count)
        {
            merged.colour_targets[i].render_target = info.colour_targets[i].render_target;
            merged.colour_targets[i].load_action   = info.colour_targets[i].load_action;
            merged.colour_targets[i].store_action  = info.colour_targets[i].store_action;
            merged.colour_targets[i].clear_value   = info.colour_targets[i].clear_value;
        }
        else
        {
            merged.colour_targets[i].render_target = nullptr;
        }
    }

    merged.num_explicit_colour_targets = num_slots;

    if (info.depth_stencil_target.depth_stencil_target)
    {
        merged.depth_stencil_target.depth_stencil_target = info.depth_stencil_target.depth_stencil_target;
    }
    else if (explicit_count > 0)
    {
        merged.depth_stencil_target.depth_stencil_target = nullptr;
    }
    if (merged.depth_stencil_target.depth_stencil_target)
    {
        merged.depth_stencil_target.load_action  = info.depth_stencil_target.load_action;
        merged.depth_stencil_target.store_action = info.depth_stencil_target.store_action;
        merged.depth_stencil_target.clear_depth  = info.depth_stencil_target.clear_depth;
    }

    m_state_cache.set_render_pass_info(merged);

    m_render_encoder = new MetalRenderCommandEncoder(
        m_command_buffer->get_buffer()->renderCommandEncoder(m_state_cache.get_render_pass_descriptor()));

    for (GRIBuffer* buf : storage_buffers)
    {
        if (!buf)
        {
            continue;
        }
        MetalBuffer* mb = static_cast<MetalBuffer*>(buf);
        m_render_encoder->get()->useResource(mb->get_buffer(), MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
    }
    for (GRITexture2D* tex : storage_textures)
    {
        if (!tex)
        {
            continue;
        }
        MetalTexture2D* mt = static_cast<MetalTexture2D*>(tex);
        m_render_encoder->get()->useResource(mt->get_texture(), MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
    }
}

void MetalCommandContext::end_render_pass()
{
    IG_CORE_ASSERT(m_render_encoder, "end_render_pass called with no open render pass");
    end_render_encoder();
}

void MetalCommandContext::begin_compute_pass(const Vector<GRIBuffer*>&    storage_bufs,
                                             const Vector<GRITexture2D*>& storage_textures)
{
    IG_CORE_ASSERT(m_compute_command_buffer, "begin_compute_pass: no active command buffer");
    IG_CORE_ASSERT(!m_compute_encoder, "begin_compute_pass: compute encoder already open");

    end_render_encoder();

    MTL::ComputeCommandEncoder* raw = m_compute_command_buffer->get_buffer()->computeCommandEncoder();
    IG_CORE_ASSERT(raw, "begin_compute_pass: failed to create compute command encoder");
    m_compute_encoder = new MetalComputeCommandEncoder(raw);

    for (GRIBuffer* buf : storage_bufs)
    {
        if (!buf)
        {
            continue;
        }
        auto* mb = static_cast<MetalBuffer*>(buf);
        m_compute_encoder->get()->useResource(mb->get_buffer(), MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
    }
    for (GRITexture2D* tex : storage_textures)
    {
        if (!tex)
        {
            continue;
        }
        auto* mt = static_cast<MetalTexture2D*>(tex);
        m_compute_encoder->get()->useResource(mt->get_texture(), MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
    }
}

void MetalCommandContext::end_compute_pass()
{
    end_compute_encoder();
}

void MetalCommandContext::set_render_targets(uint32_t num_render_targets, const GRIRenderTargetView* render_targets,
                                             const GRIDepthRenderTargetView* depth_stencil_target)
{
    IG_CORE_ASSERT(num_render_targets <= max_simultaneous_render_targets, "");
    MTL_AUTORELEASE_POOL;

    GRIDepthRenderTargetView depth_target =
        depth_stencil_target ? *depth_stencil_target : GRIDepthRenderTargetView(nullptr);

    GRIRenderTargetsInfo info(num_render_targets, render_targets, depth_target);
    set_render_targets_and_clear(info);
}

void MetalCommandContext::set_render_targets_and_clear(const GRIRenderTargetsInfo& info)
{
    GRIRenderPassInfo pending{};
    for (uint32_t i = 0; i < info.num_targets; i++)
    {
        pending.colour_targets[i].render_target     = info.colour_targets[i].texture;
        pending.colour_targets[i].mip_index         = info.colour_targets[i].mip_index;
        pending.colour_targets[i].array_slice_index = info.colour_targets[i].array_slice_index;
    }

    pending.num_explicit_colour_targets               = info.num_targets;
    pending.depth_stencil_target.depth_stencil_target = info.depth_stencil_target.texture;
    m_state_cache.set_pending_pass_info(pending);
}

void MetalCommandContext::set_vertex_buffer(GRIBuffer* buffer, uint32_t offset, uint32_t buffer_index)
{
    IG_CORE_ASSERT(m_render_encoder, "set_vertex_buffer called with no active render pass");
    IG_CORE_ASSERT(buffer, "set_vertex_buffer called with null buffer");
    MetalBuffer* metal_buffer = resource_cast<GRIBuffer>(buffer);
    m_render_encoder->get()->setVertexBuffer(metal_buffer->get_buffer(), offset, buffer_index);
}

void MetalCommandContext::set_index_buffer(GRIBuffer* buffer, GRIIndexFormat format, uint32_t offset)
{
    m_state_cache.set_index_buffer(resource_cast<GRIBuffer>(buffer)->get_buffer(),
                                   (format == GRIIndexFormat::Uint16) ? MTL::IndexTypeUInt16 : MTL::IndexTypeUInt32,
                                   offset);
}

void MetalCommandContext::draw_indexed_primitives(uint32_t index_count, uint32_t first_index, int32_t vertex_offset)
{
    IG_CORE_ASSERT(m_render_encoder, "draw_indexed_primitives called with no active render pass");
    IG_CORE_ASSERT(m_state_cache.get_index_buffer(), "draw_indexed_primitives called with no index buffer bound");

    const uint32_t index_stride =
        (m_state_cache.get_index_type() == MTL::IndexTypeUInt16) ? sizeof(uint16_t) : sizeof(uint32_t);
    const NS::UInteger byte_offset = m_state_cache.get_index_buffer_offset() + first_index * index_stride;

    m_render_encoder->get()->drawIndexedPrimitives(m_state_cache.get_primitive_type(), index_count,
                                                   m_state_cache.get_index_type(), m_state_cache.get_index_buffer(),
                                                   byte_offset, 1, vertex_offset, 0);
}

void MetalCommandContext::set_uniform_buffer(GRIBuffer* buffer, uint32_t slot, GRIShaderStage stage, uint32_t offset)
{
    MetalBuffer* metal_buffer = resource_cast<GRIBuffer>(buffer);
    if (stage == GRIShaderStage::Vertex)
    {
        IG_CORE_ASSERT(m_render_encoder, "set_uniform_buffer(Vertex): no active render encoder");
        m_render_encoder->get()->setVertexBuffer(metal_buffer->get_buffer(), offset, slot);
    }
    else if (stage == GRIShaderStage::Pixel)
    {
        IG_CORE_ASSERT(m_render_encoder, "set_uniform_buffer(Pixel): no active render encoder");
        m_render_encoder->get()->setFragmentBuffer(metal_buffer->get_buffer(), offset, slot);
    }
    else if (stage == GRIShaderStage::Compute)
    {
        IG_CORE_ASSERT(m_compute_encoder, "set_uniform_buffer(Compute): no active compute encoder");
        m_compute_encoder->get()->setBuffer(metal_buffer->get_buffer(), offset, slot);
    }
}

void MetalCommandContext::set_texture(GRITexture2D* texture, uint32_t slot, GRIShaderStage stage)
{
    MetalTexture2D* metal_tex = resource_cast<GRITexture2D>(texture);
    if (stage == GRIShaderStage::Pixel)
    {
        IG_CORE_ASSERT(m_render_encoder, "set_texture(Pixel): no active render pass");
        m_render_encoder->get()->setFragmentTexture(metal_tex->get_texture(), slot);
        m_render_encoder->get()->setFragmentSamplerState(m_default_sampler, slot);
    }
    else if (stage == GRIShaderStage::Vertex)
    {
        IG_CORE_ASSERT(m_render_encoder, "set_texture(Vertex): no active render pass");
        m_render_encoder->get()->setVertexTexture(metal_tex->get_texture(), slot);
        m_render_encoder->get()->setVertexSamplerState(m_default_sampler, slot);
    }
    else if (stage == GRIShaderStage::Compute)
    {
        IG_CORE_ASSERT(m_compute_encoder, "set_texture(Compute): no active compute pass");
        m_compute_encoder->get()->setTexture(metal_tex->get_texture(), slot);
        m_compute_encoder->get()->setSamplerState(m_default_sampler, slot);
    }
}

void MetalCommandContext::set_graphics_pipeline_state(GRIPipelineState* pipeline_state)
{
    IG_CORE_ASSERT(m_render_encoder, "set_graphics_pipeline_state called with no active render encoder");
    MetalPipelineState* pso = resource_cast<GRIPipelineState>(pipeline_state);
    m_render_encoder->get()->setRenderPipelineState(pso->get_pipeline_state());
    if (pso->get_depth_stencil_state())
    {
        m_render_encoder->get()->setDepthStencilState(pso->get_depth_stencil_state());
    }
    m_render_encoder->get()->setCullMode(pso->get_cull_mode());
    m_render_encoder->get()->setTriangleFillMode(pso->get_fill_mode());
    m_state_cache.set_primitive_type(pso->get_primitive_type());
}

void MetalCommandContext::draw_primitives(uint32_t vertex_count, uint32_t first_vertex)
{
    IG_CORE_ASSERT(m_render_encoder, "draw_primitives called with no active render encoder");
    m_render_encoder->get()->drawPrimitives(m_state_cache.get_primitive_type(), first_vertex, vertex_count);
}

void MetalCommandContext::draw_indexed_primitives_instanced(uint32_t index_count, uint32_t instance_count,
                                                            uint32_t base_instance, uint32_t first_index,
                                                            int32_t vertex_offset)
{
    IG_CORE_ASSERT(m_render_encoder, "draw_indexed_primitives_instanced called with no active render pass");
    IG_CORE_ASSERT(m_state_cache.get_index_buffer(),
                   "draw_indexed_primitives_instanced called with no index buffer bound");

    const uint32_t index_stride =
        (m_state_cache.get_index_type() == MTL::IndexTypeUInt16) ? sizeof(uint16_t) : sizeof(uint32_t);
    const NS::UInteger byte_offset = m_state_cache.get_index_buffer_offset() + first_index * index_stride;

    m_render_encoder->get()->drawIndexedPrimitives(m_state_cache.get_primitive_type(), index_count,
                                                   m_state_cache.get_index_type(), m_state_cache.get_index_buffer(),
                                                   byte_offset, instance_count, vertex_offset, base_instance);
}

void MetalCommandContext::set_compute_pipeline_state(GRIComputePipelineState* pso)
{
    IG_CORE_ASSERT(m_compute_command_buffer, "set_compute_pipeline_state called before begin_frame");
    IG_CORE_ASSERT(pso, "set_compute_pipeline_state: null PSO");

    end_render_encoder();

    if (!m_compute_encoder)
    {
        MTL::ComputeCommandEncoder* raw = m_compute_command_buffer->get_buffer()->computeCommandEncoder();
        IG_CORE_ASSERT(raw, "set_compute_pipeline_state: failed to create compute command encoder");
        m_compute_encoder = new MetalComputeCommandEncoder(raw);
    }

    auto* mpso                = static_cast<MetalComputePipelineState*>(pso);
    m_active_threadgroup_size = mpso->get_threadgroup_size();
    m_compute_encoder->get()->setComputePipelineState(mpso->get_pso());
}

void MetalCommandContext::set_storage_buffer(GRIBuffer* buffer, uint32_t slot)
{
    IG_CORE_ASSERT(m_compute_encoder, "set_storage_buffer: no active compute encoder");
    resource_cast<GRIBuffer>(buffer)->bind_storage(m_compute_encoder->get(), slot);
}

void MetalCommandContext::set_storage_texture(GRITexture2D* texture, uint32_t slot, uint32_t mip_level,
                                              uint32_t array_slice)
{
    IG_CORE_ASSERT(m_compute_encoder, "set_storage_texture: no active compute encoder");
    resource_cast<GRITexture2D>(texture)->bind_storage(m_compute_encoder->get(), slot, mip_level, array_slice);
}

void MetalCommandContext::set_compute_sampler(GRISamplerState* sampler, uint32_t slot)
{
    IG_CORE_ASSERT(m_compute_encoder, "set_compute_sampler: no active compute encoder");
    IG_CORE_ASSERT(sampler, "set_compute_sampler: null sampler");
    m_compute_encoder->get()->setSamplerState(resource_cast<GRISamplerState>(sampler)->get_sampler(), slot);
}

void MetalCommandContext::dispatch(uint32_t x, uint32_t y, uint32_t z)
{
    IG_CORE_ASSERT(m_compute_encoder, "dispatch: no active compute encoder");
    m_compute_encoder->get()->dispatchThreadgroups(MTL::Size{x, y, z}, m_active_threadgroup_size);
}

void MetalCommandContext::draw_indexed_primitives_indirect(GRIBuffer* args_buf, uint32_t byte_offset)
{
    IG_CORE_ASSERT(m_render_encoder, "draw_indexed_primitives_indirect: no active render encoder");
    IG_CORE_ASSERT(m_state_cache.get_index_buffer(), "draw_indexed_primitives_indirect: no index buffer bound");
    auto* mb = static_cast<MetalBuffer*>(args_buf);
    m_render_encoder->get()->drawIndexedPrimitives(
        m_state_cache.get_primitive_type(), m_state_cache.get_index_type(), m_state_cache.get_index_buffer(),
        m_state_cache.get_index_buffer_offset(), mb->get_buffer(), byte_offset);
}

void MetalCommandContext::memory_barrier(GRIResource* resource, GRIAccessFlags old_access, GRIAccessFlags new_access)
{
    const bool old_cw = has_flag(old_access, GRIAccessFlags::ComputeWrite);
    const bool new_compute =
        has_flag(new_access, GRIAccessFlags::ComputeWrite) || has_flag(new_access, GRIAccessFlags::ComputeRead);

    if (old_cw && new_compute && m_compute_encoder)
    {
        m_compute_encoder->get()->memoryBarrier(MTL::BarrierScopeBuffers | MTL::BarrierScopeTextures);
        return;
    }

    end_compute_encoder();
    end_render_encoder();
}

} // namespace Ignis
