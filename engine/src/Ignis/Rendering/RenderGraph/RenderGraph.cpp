#include "igpch.h"
#include "RenderGraph.h"
#include <Ignis/Rendering/GRI/GRI.h>

namespace Ignis
{

static constexpr uint64_t fnv1a_64(const char* data, size_t size)
{
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < size; ++i)
        h = (h ^ static_cast<uint8_t>(data[i])) * 1099511628211ULL;
    return h;
}

RenderGraph::RenderGraph(size_t arena_size)
    : m_arena(arena_size)
{}

RenderGraph::~RenderGraph()
{
    for (RGPassBase* pass : m_passes)
        pass->run_destructor();
}

// --- Public API ---

void RenderGraph::reset()
{
    m_pool.begin_frame(m_frame_index++);
    m_arena.soft_reset();

    m_passes.clear();
    m_textures.clear();
    m_buffers.clear();
    m_resolved_tex.clear();
    m_resolved_buf.clear();
    m_sorted_passes.clear();
    m_current_fingerprint = 0;
}

const char* RenderGraph::intern_string(const char* src)
{
    if (!src) return nullptr;
    size_t len  = strlen(src);
    char*  copy = static_cast<char*>(m_arena.allocate(len + 1));
    std::memcpy(copy, src, len + 1);
    return copy;
}

// --- Resource Registration ---

RGTextureHandle RenderGraph::register_texture(RGInternal::VirtualTexture&& vt)
{
    IG_CORE_ASSERT(m_textures.size() < k_rg_invalid_id, "Virtual texture table full");
    RGTextureHandle h;
    h.id = static_cast<uint16_t>(m_textures.size());
    m_textures.push_back(std::move(vt));
    return h;
}

RGInternal::VirtualTexture& RenderGraph::get_virtual_texture(uint16_t id)
{
    IG_CORE_ASSERT(id < (uint16_t)m_textures.size(), "Texture id out of range");
    return m_textures[id];
}

const RGInternal::VirtualTexture& RenderGraph::get_virtual_texture(uint16_t id) const
{
    IG_CORE_ASSERT(id < (uint16_t)m_textures.size(), "Texture id out of range");
    return m_textures[id];
}

RGBufferHandle RenderGraph::register_buffer(RGInternal::VirtualBuffer&& vb)
{
    IG_CORE_ASSERT(m_buffers.size() < k_rg_invalid_id, "Virtual buffer table full");
    RGBufferHandle h;
    h.id = static_cast<uint16_t>(m_buffers.size());
    m_buffers.push_back(std::move(vb));
    return h;
}

RGInternal::VirtualBuffer& RenderGraph::get_virtual_buffer(uint16_t id)
{
    IG_CORE_ASSERT(id < (uint16_t)m_buffers.size(), "Buffer id out of range");
    return m_buffers[id];
}

const RGInternal::VirtualBuffer& RenderGraph::get_virtual_buffer(uint16_t id) const
{
    IG_CORE_ASSERT(id < (uint16_t)m_buffers.size(), "Buffer id out of range");
    return m_buffers[id];
}

// --- Fingerprinting ---

void RenderGraph::fold_pass_into_fingerprint(const RGPassBase& pass, const char* name)
{
    uint64_t h = fnv1a_64(name, strlen(name));
    h ^= static_cast<uint64_t>(pass.num_color_slots) * 2654435761ULL;
    h ^= static_cast<uint64_t>(pass.has_depth)       * 2246822519ULL;
    h ^= static_cast<uint64_t>(pass.texture_reads.size())  << 8;
    h ^= static_cast<uint64_t>(pass.buffer_reads.size())   << 16;
    h ^= static_cast<uint64_t>(pass.texture_writes.size()) << 24;
    h ^= static_cast<uint64_t>(pass.buffer_writes.size())  << 32;
    m_current_fingerprint ^= h;
}

// --- Compilation ---

void RenderGraph::compile()
{
    // Phase 1: Stamp writer_pass_idx for every resource written by each pass.
    for (uint16_t pass_idx = 0; pass_idx < (uint16_t)m_passes.size(); ++pass_idx)
    {
        const RGPassBase& p = *m_passes[pass_idx];

        for (uint32_t s = 0; s < p.num_color_slots; ++s)
        {
            uint16_t tid = p.color_slots[s].texture_id;
            if (tid != k_rg_invalid_id)
            {
                IG_CORE_ASSERT(m_textures[tid].writer_pass_idx == k_rg_invalid_id,
                    "RenderGraph: multiple writers to the same texture are not permitted");
                m_textures[tid].writer_pass_idx = pass_idx;
            }
        }

        if (p.has_depth && p.depth_slot.texture_id != k_rg_invalid_id)
        {
            IG_CORE_ASSERT(m_textures[p.depth_slot.texture_id].writer_pass_idx == k_rg_invalid_id,
                "RenderGraph: multiple writers to the same depth texture are not permitted");
            m_textures[p.depth_slot.texture_id].writer_pass_idx = pass_idx;
        }

        for (uint16_t twid : p.texture_writes)
        {
            IG_CORE_ASSERT(m_textures[twid].writer_pass_idx == k_rg_invalid_id,
                "RenderGraph: multiple writers to the same storage texture are not permitted");
            m_textures[twid].writer_pass_idx = pass_idx;
        }

        for (uint16_t bid : p.buffer_writes)
        {
            IG_CORE_ASSERT(m_buffers[bid].writer_pass_idx == k_rg_invalid_id,
                "RenderGraph: multiple writers to the same buffer are not permitted");
            m_buffers[bid].writer_pass_idx = pass_idx;
        }
    }

    // Phase 2: Reverse BFS from imported (root) resources → mark live passes.
    Vector<uint8_t>  pass_live(m_passes.size(), 0);
    Vector<uint16_t> tex_queue;
    Vector<uint16_t> buf_queue;

    for (uint16_t tid = 0; tid < (uint16_t)m_textures.size(); ++tid)
        if (m_textures[tid].is_imported) tex_queue.push_back(tid);

    for (uint16_t bid = 0; bid < (uint16_t)m_buffers.size(); ++bid)
        if (m_buffers[bid].is_imported) buf_queue.push_back(bid);

    auto try_mark_pass = [&](uint16_t writer)
    {
        if (writer == k_rg_invalid_id || pass_live[writer]) return;
        pass_live[writer] = 1;
        for (uint16_t rtid : m_passes[writer]->texture_reads) tex_queue.push_back(rtid);
        for (uint16_t rbid : m_passes[writer]->buffer_reads)  buf_queue.push_back(rbid);
    };

    while (!tex_queue.empty() || !buf_queue.empty())
    {
        while (!tex_queue.empty())
        {
            uint16_t tid = tex_queue.back(); tex_queue.pop_back();
            try_mark_pass(m_textures[tid].writer_pass_idx);
        }
        while (!buf_queue.empty())
        {
            uint16_t bid = buf_queue.back(); buf_queue.pop_back();
            try_mark_pass(m_buffers[bid].writer_pass_idx);
        }
    }

    // Phase 3: Stamp is_culled.
    for (size_t i = 0; i < m_passes.size(); ++i)
        m_passes[i]->is_culled = !pass_live[i];

#if defined(IG_DEBUG)
    for (uint16_t i = 0; i < (uint16_t)m_passes.size(); ++i)
    {
        if (m_passes[i]->is_culled) continue;
        for (uint16_t rtid : m_passes[i]->texture_reads)
        {
            uint16_t w = m_textures[rtid].writer_pass_idx;
            IG_CORE_ASSERT(
                m_textures[rtid].is_imported || w == k_rg_invalid_id || w < i,
                "RenderGraph: pass reads a texture whose producer is declared after it");
        }
        for (uint16_t rbid : m_passes[i]->buffer_reads)
        {
            uint16_t w = m_buffers[rbid].writer_pass_idx;
            IG_CORE_ASSERT(
                m_buffers[rbid].is_imported || w == k_rg_invalid_id || w < i,
                "RenderGraph: pass reads a buffer whose producer is declared after it");
        }
    }
#endif

    // Phase 4: Compute [first_used_pass, last_used_pass] for all live virtual resources.
    for (uint16_t i = 0; i < (uint16_t)m_passes.size(); ++i)
    {
        if (m_passes[i]->is_culled) continue;
        const RGPassBase& p = *m_passes[i];

        auto update_tex = [&](uint16_t tid, bool is_write)
        {
            if (tid == k_rg_invalid_id) return;
            RGInternal::VirtualTexture& vt = m_textures[tid];
            if (is_write && (vt.first_used_pass == k_rg_invalid_id || i < vt.first_used_pass))
                vt.first_used_pass = i;
            if (vt.last_used_pass == k_rg_invalid_id || i > vt.last_used_pass)
                vt.last_used_pass = i;
        };

        auto update_buf = [&](uint16_t bid, bool is_write)
        {
            if (bid == k_rg_invalid_id) return;
            RGInternal::VirtualBuffer& vb = m_buffers[bid];
            if (is_write && (vb.first_used_pass == k_rg_invalid_id || i < vb.first_used_pass))
                vb.first_used_pass = i;
            if (vb.last_used_pass == k_rg_invalid_id || i > vb.last_used_pass)
                vb.last_used_pass = i;
        };

        for (uint32_t s = 0; s < p.num_color_slots; ++s)
            update_tex(p.color_slots[s].texture_id, true);
        if (p.has_depth)
            update_tex(p.depth_slot.texture_id, true);
        for (uint16_t rtid : p.texture_reads)  update_tex(rtid, false);
        for (uint16_t twid : p.texture_writes)  update_tex(twid, true);
        for (uint16_t rbid : p.buffer_reads)   update_buf(rbid, false);
        for (uint16_t wbid : p.buffer_writes)  update_buf(wbid, true);
    }

    // Phase 5: Pool-based physical allocation — alias where lifetimes don't overlap.
    for (size_t i = 0; i < m_textures.size(); ++i)
    {
        RGInternal::VirtualTexture& vt = m_textures[i];
        if (vt.is_imported || vt.physical) continue;

        uint16_t writer = vt.writer_pass_idx;
        if (writer == k_rg_invalid_id || m_passes[writer]->is_culled) continue;

        GRITexture2DDesc desc;
        desc.width          = vt.desc.width;
        desc.height         = vt.desc.height;
        desc.format         = vt.desc.format;
        desc.num_mip_levels = vt.desc.num_mip_levels;
        vt.physical = m_pool.acquire(desc, vt.first_used_pass, vt.last_used_pass, m_frame_index);
    }

    for (size_t i = 0; i < m_buffers.size(); ++i)
    {
        RGInternal::VirtualBuffer& vb = m_buffers[i];
        if (vb.is_imported || vb.physical) continue;

        uint16_t writer = vb.writer_pass_idx;
        if (writer == k_rg_invalid_id || m_passes[writer]->is_culled) continue;

        GRIBufferDesc desc;
        desc.size  = vb.desc.size;
        desc.usage = vb.desc.usage;
        vb.physical = m_pool.acquire_buffer(desc, vb.first_used_pass, vb.last_used_pass, m_frame_index);
    }

    m_resolved_tex.resize(m_textures.size());
    for (uint16_t i = 0; i < (uint16_t)m_textures.size(); ++i)
        m_resolved_tex[i] = m_textures[i].physical;

    m_resolved_buf.resize(m_buffers.size());
    for (uint16_t i = 0; i < (uint16_t)m_buffers.size(); ++i)
        m_resolved_buf[i] = m_buffers[i].physical;

    // Phase 6: Kahn's BFS topological sort over non-culled passes.
    {
        const uint16_t n = static_cast<uint16_t>(m_passes.size());
        Vector<uint32_t>         in_degree(n, 0);
        Vector<Vector<uint16_t>> successors(n);
        uint16_t                 live_count = 0;

        for (uint16_t i = 0; i < n; ++i)
        {
            if (m_passes[i]->is_culled) continue;
            ++live_count;

            auto add_edge = [&](uint16_t writer)
            {
                if (writer == k_rg_invalid_id || m_passes[writer]->is_culled || writer == i) return;
                successors[writer].push_back(i);
                ++in_degree[i];
            };

            for (uint16_t rtid : m_passes[i]->texture_reads)
                add_edge(m_textures[rtid].writer_pass_idx);
            for (uint16_t rbid : m_passes[i]->buffer_reads)
                add_edge(m_buffers[rbid].writer_pass_idx);
        }

        Vector<uint16_t> q;
        q.reserve(live_count);
        for (uint16_t i = 0; i < n; ++i)
        {
            if (!m_passes[i]->is_culled && in_degree[i] == 0)
                q.push_back(i);
        }

        m_sorted_passes.reserve(live_count);
        while (!q.empty())
        {
            uint16_t idx = q.back(); q.pop_back();
            m_sorted_passes.push_back(idx);
            for (uint16_t succ : successors[idx])
            {
                if (--in_degree[succ] == 0)
                    q.push_back(succ);
            }
        }

        IG_CORE_ASSERT(m_sorted_passes.size() == live_count, "RenderGraph: dependency cycle detected");
    }

    m_topo_cache.last_fingerprint = m_current_fingerprint;
}

GRIRenderPassInfo RenderGraph::build_pass_info(const RGPassBase& pass) const
{
    GRIRenderPassInfo info;

    for (uint32_t s = 0; s < pass.num_color_slots; ++s)
    {
        const RGInternal::AttachmentSlot& slot = pass.color_slots[s];
        if (slot.texture_id == k_rg_invalid_id) continue;

        GRIRenderPassInfo::ColourEntry& entry = info.colour_targets[s];
        entry.render_target = m_resolved_tex[slot.texture_id];
        entry.load_action   = slot.load_action;
        entry.store_action  = slot.store_action;
        entry.clear_value   = slot.clear_value;
    }

    if (pass.has_depth && pass.depth_slot.texture_id != k_rg_invalid_id)
    {
        auto& d                = info.depth_stencil_target;
        d.depth_stencil_target = m_resolved_tex[pass.depth_slot.texture_id];
        d.load_action          = pass.depth_slot.load_action;
        d.store_action         = pass.depth_slot.store_action;
        d.clear_depth          = pass.depth_slot.clear_depth;
    }

    return info;
}

} // namespace Ignis
