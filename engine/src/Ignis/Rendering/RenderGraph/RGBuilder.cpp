#include "igpch.h"
#include "RGBuilder.h"
#include "RenderGraph.h"
#include <Ignis/Rendering/GRI/GRI.h>
#include <Ignis/Rendering/GRI/GRICommandList.h>
#include <Ignis/Rendering/Renderer.h>

namespace Ignis
{

RGBuilder::RGBuilder()
{
    m_current_deps.reset();
}

// --- Resource Declaration ---

RGTextureHandle RGBuilder::create_texture(const char* name, const RGTextureDesc& desc)
{
    RGInternal::VirtualTexture vt;
    vt.name        = m_graph.intern_string(name);
    vt.desc        = desc;
    vt.is_imported = false;
    return m_graph.register_texture(std::move(vt));
}

RGTextureHandle RGBuilder::import_backbuffer()
{
    RGInternal::VirtualTexture vt;
    vt.name        = "Backbuffer";
    vt.physical    = nullptr; // Metal backend resolves nullptr → current drawable
    vt.is_imported = true;
    vt.ref_count   = 1;
    return m_graph.register_texture(std::move(vt));
}

RGTextureHandle RGBuilder::import_viewport_depth()
{
    GRITexture2D* depth = Renderer::get_depth_texture();
    IG_CORE_ASSERT(depth, "import_viewport_depth: no viewport depth texture — call after Renderer::begin_frame");
    return import_texture("ViewportDepth", depth);
}

RGTextureHandle RGBuilder::import_texture(const char* name, GRITexture2D* physical)
{
    IG_CORE_ASSERT(physical, "import_texture requires a valid physical pointer");
    RGInternal::VirtualTexture vt;
    vt.name        = m_graph.intern_string(name);
    vt.physical    = physical;
    vt.is_imported = true;
    vt.ref_count   = 1;
    return m_graph.register_texture(std::move(vt));
}

RGBufferHandle RGBuilder::create_buffer(const char* name, const RGBufferDesc& desc)
{
    RGInternal::VirtualBuffer vb;
    vb.name        = m_graph.intern_string(name);
    vb.desc        = desc;
    vb.is_imported = false;
    return m_graph.register_buffer(std::move(vb));
}

RGBufferHandle RGBuilder::import_buffer(const char* name, GRIBuffer* physical)
{
    IG_CORE_ASSERT(physical, "import_buffer requires a valid physical pointer");
    RGInternal::VirtualBuffer vb;
    vb.name        = m_graph.intern_string(name);
    vb.physical    = physical;
    vb.is_imported = true;
    vb.ref_count   = 1;
    return m_graph.register_buffer(std::move(vb));
}

// --- Stateful Dependency Bindings ---

void RGBuilder::read_texture(RGTextureHandle h)
{
    IG_CORE_ASSERT(h.is_valid(), "read_texture: invalid handle");
    m_graph.get_virtual_texture(h.id).ref_count++;
    m_current_deps.texture_reads.push_back(h.id);
}

void RGBuilder::write_render_target(uint32_t slot, RGTextureHandle h, const RGColorAttachmentDesc& desc)
{
    IG_CORE_ASSERT(h.is_valid(), "write_render_target: invalid handle");
    IG_CORE_ASSERT(slot < max_simultaneous_render_targets, "Color slot out of range");

    RGInternal::AttachmentSlot& s = m_current_deps.color_slots[slot];
    s.texture_id                  = h.id;
    s.load_action                 = desc.load_action;
    s.store_action                = desc.store_action;
    s.clear_value                 = desc.clear_value;

    if (slot + 1 > m_current_deps.num_color_slots)
    {
        m_current_deps.num_color_slots = slot + 1;
    }
}

void RGBuilder::write_depth_stencil(RGTextureHandle h, const RGDepthAttachmentDesc& desc)
{
    IG_CORE_ASSERT(h.is_valid(), "write_depth_stencil: invalid handle");

    m_current_deps.depth_slot.texture_id   = h.id;
    m_current_deps.depth_slot.load_action  = desc.load_action;
    m_current_deps.depth_slot.store_action = desc.store_action;
    m_current_deps.depth_slot.clear_depth  = desc.clear_depth;
    m_current_deps.has_depth               = true;
}

void RGBuilder::read_depth_stencil(RGTextureHandle h, const RGDepthAttachmentDesc& desc)
{
    IG_CORE_ASSERT(h.is_valid(), "read_depth_stencil: invalid handle");

    m_current_deps.depth_slot.texture_id   = h.id;
    m_current_deps.depth_slot.load_action  = desc.load_action;
    m_current_deps.depth_slot.store_action = desc.store_action;
    m_current_deps.depth_slot.clear_depth  = desc.clear_depth;
    m_current_deps.has_depth               = true;
    m_current_deps.depth_read_only         = true;
}

void RGBuilder::write_storage_texture(RGTextureHandle h)
{
    IG_CORE_ASSERT(h.is_valid(), "write_storage_texture: invalid handle");
    m_current_deps.texture_writes.push_back(h.id);
}

void RGBuilder::read_buffer(RGBufferHandle h)
{
    IG_CORE_ASSERT(h.is_valid(), "read_buffer: invalid handle");
    m_graph.get_virtual_buffer(h.id).ref_count++;
    m_current_deps.buffer_reads.push_back(h.id);
}

void RGBuilder::write_buffer(RGBufferHandle h)
{
    IG_CORE_ASSERT(h.is_valid(), "write_buffer: invalid handle");
    m_current_deps.buffer_writes.push_back(h.id);
}

// --- Physical Resource Access ---

GRITexture2D* RGBuilder::get_physical(RGTextureHandle h) const
{
    IG_CORE_ASSERT(h.is_valid(), "get_physical: invalid RGTextureHandle");
    IG_CORE_ASSERT(h.id < (uint16_t)m_graph.m_resolved_tex.size(), "RGTextureHandle out of range");
    return m_graph.m_resolved_tex[h.id];
}

GRIBuffer* RGBuilder::get_physical(RGBufferHandle h) const
{
    IG_CORE_ASSERT(h.is_valid(), "get_physical: invalid RGBufferHandle");
    IG_CORE_ASSERT(h.id < (uint16_t)m_graph.m_resolved_buf.size(), "RGBufferHandle out of range");
    return m_graph.m_resolved_buf[h.id];
}

// --- Frame Pipeline ---

void RGBuilder::execute(GRICommandList& cmd)
{
    m_graph.compile();

    for (uint16_t idx : m_graph.m_sorted_passes)
    {
        RGPassBase*       pass = m_graph.m_passes[idx];
        GRIRenderPassInfo info = m_graph.build_pass_info(*pass);
        cmd.begin_render_pass(info);
        pass->run_execute(cmd);
        cmd.end_render_pass();
    }

    for (RGPassBase* pass : m_graph.m_passes)
    {
        pass->run_destructor();
    }

    m_graph.reset();
}

// --- Private Helpers ---

void* RGBuilder::arena_alloc(size_t size, size_t alignment)
{
    return m_graph.m_arena.allocate(size, alignment);
}

void RGBuilder::commit_pass(const char* name, RGPassBase* pass)
{
    pass->name            = m_graph.intern_string(name);
    pass->texture_reads   = std::move(m_current_deps.texture_reads);
    pass->buffer_reads    = std::move(m_current_deps.buffer_reads);
    pass->texture_writes  = std::move(m_current_deps.texture_writes);
    pass->buffer_writes   = std::move(m_current_deps.buffer_writes);
    pass->num_color_slots = m_current_deps.num_color_slots;
    pass->has_depth       = m_current_deps.has_depth;
    pass->depth_read_only = m_current_deps.depth_read_only;
    pass->depth_slot      = m_current_deps.depth_slot;
    std::memcpy(pass->color_slots, m_current_deps.color_slots, sizeof(pass->color_slots));

    m_graph.m_passes.push_back(pass);
    m_graph.fold_pass_into_fingerprint(*pass, name);
    m_current_deps.reset();
}

void RGBuilder::PassDependencies::reset()
{
    texture_reads.clear();
    buffer_reads.clear();
    texture_writes.clear();
    buffer_writes.clear();
    for (auto& s : color_slots)
    {
        s            = {};
        s.texture_id = k_rg_invalid_id;
    }
    depth_slot      = {};
    num_color_slots = 0;
    has_depth       = false;
    depth_read_only = false;
}

} // namespace Ignis
