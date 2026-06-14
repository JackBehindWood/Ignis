#include "igpch.h"
#include "RGBuilder.h"
#include "RenderGraph.h"
#include <Ignis/Rendering/GRI/GRI.h>
#include <Ignis/Rendering/GRI/GRICommandList.h>
#include <Ignis/Rendering/GRI/GRIDefinitions.h>
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

RGBufferHandle RGBuilder::import_buffer(const char* name, GRIBuffer* physical, GRIBufferUsage usage)
{
    IG_CORE_ASSERT(physical, "import_buffer requires a valid physical pointer");
    RGInternal::VirtualBuffer vb;
    vb.name        = m_graph.intern_string(name);
    vb.physical    = physical;
    vb.is_imported = true;
    vb.ref_count   = 1;
    vb.desc.usage  = usage;
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
    m_current_deps.storage_texture_writes.push_back(h.id);
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

void RGBuilder::read_storage_buffer(RGBufferHandle h)
{
    IG_CORE_ASSERT(h.is_valid(), "read_storage_buffer: invalid handle");
    m_graph.get_virtual_buffer(h.id).ref_count++;
    m_current_deps.storage_buffer_reads.push_back(h.id);
}

void RGBuilder::write_storage_buffer(RGBufferHandle h)
{
    IG_CORE_ASSERT(h.is_valid(), "write_storage_buffer: invalid handle");
    m_current_deps.storage_buffer_writes.push_back(h.id);
}

void RGBuilder::read_storage_texture(RGTextureHandle h)
{
    IG_CORE_ASSERT(h.is_valid(), "read_storage_texture: invalid handle");
    m_graph.get_virtual_texture(h.id).ref_count++;
    m_current_deps.storage_texture_reads.push_back(h.id);
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

// Returns true if every barrier in the slot is WAW (ComputeWrite → ComputeWrite).
// An empty barrier list is trivially WAW — no hazard, encoder may remain open.
static bool all_barriers_waw(const Vector<RenderGraph::RGBarrier>& barriers)
{
    for (const RenderGraph::RGBarrier& b : barriers)
    {
        if (!has_flag(b.old_access, GRIAccessFlags::ComputeWrite) ||
            !has_flag(b.new_access, GRIAccessFlags::ComputeWrite))
        {
            return false;
        }
    }
    return true;
}

void RGBuilder::execute(GRICommandList& cmd)
{
    m_graph.compile();

    const size_t pass_count         = m_graph.m_sorted_passes.size();
    bool         compute_chain_open = false;

    for (size_t si = 0; si < pass_count; ++si)
    {
        // Emit pre-pass barriers into the deferred command list.
        // WAW barriers fire while the compute encoder is still open (intra-encoder path).
        // All other barriers find the encoder already closed (no-op for encoder management;
        // Metal's endEncoding from end_compute_pass already provided coherence).
        for (const RenderGraph::RGBarrier& b : m_graph.m_sorted_barriers[si])
        {
            cmd.memory_barrier(b.resource, b.old_access, b.new_access);
        }

        const uint16_t idx  = m_graph.m_sorted_passes[si];
        RGPassBase&    pass = *m_graph.m_passes[idx];

        if (pass.pass_type == RGPassType::Compute)
        {
            if (!compute_chain_open)
            {
                // Open one compute encoder for this pass and all subsequent WAW-chained
                // compute passes, declaring the union of their resources up-front so that
                // Metal's useResource sweep covers every buffer/texture in the chain.
                Vector<GRIBuffer*>    chain_bufs;
                Vector<GRITexture2D*> chain_texs;

                for (size_t csi = si; csi < pass_count; ++csi)
                {
                    const RGPassBase& cp = *m_graph.m_passes[m_graph.m_sorted_passes[csi]];
                    if (cp.pass_type != RGPassType::Compute)
                    {
                        break;
                    }

                    for (uint16_t bid : cp.storage_buffer_reads)
                    {
                        if (GRIBuffer* p = m_graph.m_resolved_buf[bid])
                        {
                            chain_bufs.push_back(p);
                        }
                    }
                    for (uint16_t bid : cp.storage_buffer_writes)
                    {
                        if (GRIBuffer* p = m_graph.m_resolved_buf[bid])
                        {
                            chain_bufs.push_back(p);
                        }
                    }
                    for (uint16_t tid : cp.storage_texture_reads)
                    {
                        if (GRITexture2D* p = m_graph.m_resolved_tex[tid])
                        {
                            chain_texs.push_back(p);
                        }
                    }
                    for (uint16_t tid : cp.storage_texture_writes)
                    {
                        if (GRITexture2D* p = m_graph.m_resolved_tex[tid])
                        {
                            chain_texs.push_back(p);
                        }
                    }

                    // Stop collecting if the next pass breaks the WAW chain.
                    if (csi + 1 >= pass_count)
                    {
                        break;
                    }
                    const RGPassBase& np = *m_graph.m_passes[m_graph.m_sorted_passes[csi + 1]];
                    if (np.pass_type != RGPassType::Compute)
                    {
                        break;
                    }
                    if (!all_barriers_waw(m_graph.m_sorted_barriers[csi + 1]))
                    {
                        break;
                    }
                }

                cmd.begin_compute_pass(std::move(chain_bufs), std::move(chain_texs));
                compute_chain_open = true;
            }

            pass.run_execute(cmd);

            // Keep the encoder open only if the next pass continues this WAW chain.
            bool keep_chain = false;
            if (si + 1 < pass_count)
            {
                const RGPassBase& next_pass = *m_graph.m_passes[m_graph.m_sorted_passes[si + 1]];
                keep_chain =
                    next_pass.pass_type == RGPassType::Compute && all_barriers_waw(m_graph.m_sorted_barriers[si + 1]);
            }

            if (!keep_chain)
            {
                cmd.end_compute_pass();
                compute_chain_open = false;
            }
        }
        else
        {
            // Graphics pass: close any open compute chain first so begin_render_pass
            // always opens a fresh render encoder with no competing encoder active.
            if (compute_chain_open)
            {
                cmd.end_compute_pass();
                compute_chain_open = false;
            }

            // Assemble MetalPassResourceList: storage buffer/texture deps + IndirectBuffer reads.
            // begin_render_pass sweeps this list with useResource so Metal schedules all draws.
            Vector<GRIBuffer*>    res_bufs;
            Vector<GRITexture2D*> res_texs;

            for (uint16_t bid : pass.storage_buffer_reads)
            {
                if (GRIBuffer* p = m_graph.m_resolved_buf[bid])
                {
                    res_bufs.push_back(p);
                }
            }
            for (uint16_t bid : pass.storage_buffer_writes)
            {
                if (GRIBuffer* p = m_graph.m_resolved_buf[bid])
                {
                    res_bufs.push_back(p);
                }
            }
            for (uint16_t bid : pass.buffer_reads)
            {
                if (has_flag(m_graph.m_buffers[bid].desc.usage, GRIBufferUsage::IndirectBuffer))
                {
                    if (GRIBuffer* p = m_graph.m_resolved_buf[bid])
                    {
                        res_bufs.push_back(p);
                    }
                }
            }
            for (uint16_t tid : pass.storage_texture_reads)
            {
                if (GRITexture2D* p = m_graph.m_resolved_tex[tid])
                {
                    res_texs.push_back(p);
                }
            }
            for (uint16_t tid : pass.storage_texture_writes)
            {
                if (GRITexture2D* p = m_graph.m_resolved_tex[tid])
                {
                    res_texs.push_back(p);
                }
            }

            GRIRenderPassInfo info = m_graph.build_pass_info(pass);
            cmd.begin_render_pass(info, std::move(res_bufs), std::move(res_texs));
            pass.run_execute(cmd);
            cmd.end_render_pass();
        }
    }

    IG_CORE_ASSERT(!compute_chain_open, "RenderGraph: compute chain still open at end of frame");

    m_graph.reset();
}

// --- Private Helpers ---

void* RGBuilder::arena_alloc(size_t size, size_t alignment)
{
    return m_graph.m_arena.allocate(size, alignment);
}

void RGBuilder::commit_pass(const char* name, RGPassBase* pass, RGPassType type)
{
    pass->pass_type              = type;
    pass->name                   = m_graph.intern_string(name);
    pass->texture_reads          = std::move(m_current_deps.texture_reads);
    pass->buffer_reads           = std::move(m_current_deps.buffer_reads);
    pass->texture_writes         = std::move(m_current_deps.texture_writes);
    pass->buffer_writes          = std::move(m_current_deps.buffer_writes);
    pass->storage_buffer_reads   = std::move(m_current_deps.storage_buffer_reads);
    pass->storage_buffer_writes  = std::move(m_current_deps.storage_buffer_writes);
    pass->storage_texture_reads  = std::move(m_current_deps.storage_texture_reads);
    pass->storage_texture_writes = std::move(m_current_deps.storage_texture_writes);
    pass->num_color_slots        = m_current_deps.num_color_slots;
    pass->has_depth              = m_current_deps.has_depth;
    pass->depth_read_only        = m_current_deps.depth_read_only;
    pass->depth_slot             = m_current_deps.depth_slot;
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
    storage_buffer_reads.clear();
    storage_buffer_writes.clear();
    storage_texture_reads.clear();
    storage_texture_writes.clear();
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
