#include "igpch.h"
#include "RenderGraph.h"
#include <Ignis/Rendering/GRI/GRI.h>

namespace Ignis
{

static constexpr uint64_t fnv1a_64(const char* data, size_t size)
{
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < size; ++i)
    {
        h = (h ^ static_cast<uint8_t>(data[i])) * 1099511628211ULL;
    }
    return h;
}

// Phase 3.5 helpers — map virtual resource access to GRIAccessFlags.

static GRIAccessFlags get_buffer_access(const RGPassBase& pass, uint16_t bid, GRIBufferUsage usage)
{
    if (pass.pass_type == RGPassType::Compute)
    {
        for (uint16_t id : pass.storage_buffer_writes)
        {
            if (id == bid)
            {
                return GRIAccessFlags::ComputeWrite;
            }
        }
        for (uint16_t id : pass.storage_buffer_reads)
        {
            if (id == bid)
            {
                return GRIAccessFlags::ComputeRead;
            }
        }
    }
    for (uint16_t id : pass.buffer_reads)
    {
        if (id == bid)
        {
            return has_flag(usage, GRIBufferUsage::IndirectBuffer) ? GRIAccessFlags::IndirectCommandRead
                                                                   : GRIAccessFlags::ComputeRead;
        }
    }
    for (uint16_t id : pass.buffer_writes)
    {
        if (id == bid)
        {
            return GRIAccessFlags::ComputeWrite;
        }
    }
    return GRIAccessFlags::None;
}

static GRIAccessFlags get_texture_access(const RGPassBase& pass, uint16_t tid)
{
    if (pass.pass_type == RGPassType::Compute)
    {
        for (uint16_t id : pass.storage_texture_writes)
        {
            if (id == tid)
            {
                return GRIAccessFlags::ComputeWrite;
            }
        }
        for (uint16_t id : pass.storage_texture_reads)
        {
            if (id == tid)
            {
                return GRIAccessFlags::ComputeRead;
            }
        }
    }
    for (uint16_t id : pass.texture_reads)
    {
        if (id == tid)
        {
            return GRIAccessFlags::GraphicsShaderRead;
        }
    }
    for (uint32_t s = 0; s < pass.num_color_slots; ++s)
    {
        if (pass.color_slots[s].texture_id == tid)
        {
            return GRIAccessFlags::RenderTarget;
        }
    }
    if (pass.has_depth && pass.depth_slot.texture_id == tid)
    {
        return pass.depth_read_only ? GRIAccessFlags::GraphicsShaderRead : GRIAccessFlags::DepthStencilWrite;
    }
    for (uint16_t id : pass.texture_writes)
    {
        if (id == tid)
        {
            return GRIAccessFlags::ComputeWrite;
        }
    }
    return GRIAccessFlags::None;
}

static bool is_hazard(GRIAccessFlags a, GRIAccessFlags b)
{
    if (a == GRIAccessFlags::None || b == GRIAccessFlags::None)
    {
        return false;
    }
    const bool a_write = has_flag(a, GRIAccessFlags::ComputeWrite) || has_flag(a, GRIAccessFlags::RenderTarget) ||
                         has_flag(a, GRIAccessFlags::DepthStencilWrite);
    const bool b_write = has_flag(b, GRIAccessFlags::ComputeWrite) || has_flag(b, GRIAccessFlags::RenderTarget) ||
                         has_flag(b, GRIAccessFlags::DepthStencilWrite);
    return a_write || b_write;
}

RenderGraph::RenderGraph(size_t arena_size)
    : m_arena(arena_size)
{
}

RenderGraph::~RenderGraph()
{
    for (RGPassBase* pass : m_passes)
    {
        pass->run_destructor();
    }
}

// --- Public API ---

void RenderGraph::reset()
{
    m_pool.begin_frame(m_frame_index++);

    for (RGPassBase* pass : m_passes)
    {
        if (pass)
        {
            pass->run_destructor();
        }
    }

    m_arena.soft_reset();

    m_passes.clear();
    m_textures.clear();
    m_buffers.clear();
    m_resolved_tex.clear();
    m_resolved_buf.clear();
    m_sorted_passes.clear();
    m_sorted_barriers.clear();
    m_current_fingerprint = 0;
}

const char* RenderGraph::intern_string(const char* src)
{
    if (!src)
    {
        return nullptr;
    }
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
    h ^= static_cast<uint64_t>(pass.has_depth) * 2246822519ULL;
    h ^= static_cast<uint64_t>(pass.pass_type == RGPassType::Compute) * 1597334677ULL;
    h ^= static_cast<uint64_t>(pass.texture_reads.size()) << 8;
    h ^= static_cast<uint64_t>(pass.buffer_reads.size()) << 16;
    h ^= static_cast<uint64_t>(pass.texture_writes.size()) << 24;
    h ^= static_cast<uint64_t>(pass.buffer_writes.size()) << 32;
    h ^= static_cast<uint64_t>(pass.storage_buffer_reads.size()) << 40;
    h ^= static_cast<uint64_t>(pass.storage_buffer_writes.size()) << 48;
    m_current_fingerprint ^= h;
}

// --- Compilation ---

void RenderGraph::compile()
{
    const uint16_t           n = static_cast<uint16_t>(m_passes.size());
    Vector<Vector<uint16_t>> waw_preds(n);

    // Phase 1: Stamp writer_pass_idx; record write-after-write predecessor chains.
    for (uint16_t pass_idx = 0; pass_idx < n; ++pass_idx)
    {
        const RGPassBase& p = *m_passes[pass_idx];

        auto stamp_tex = [&](uint16_t tid)
        {
            if (tid == k_rg_invalid_id)
            {
                return;
            }
            uint16_t prev = m_textures[tid].writer_pass_idx;
            if (prev != k_rg_invalid_id)
            {
                waw_preds[pass_idx].push_back(prev);
            }
            m_textures[tid].writer_pass_idx = pass_idx;
        };

        for (uint32_t s = 0; s < p.num_color_slots; ++s)
        {
            stamp_tex(p.color_slots[s].texture_id);
        }

        if (p.has_depth && !p.depth_read_only)
        {
            stamp_tex(p.depth_slot.texture_id);
        }

        for (uint16_t twid : p.texture_writes)
        {
            stamp_tex(twid);
        }
        for (uint16_t twid : p.storage_texture_writes)
        {
            stamp_tex(twid);
        }

        for (uint16_t bid : p.buffer_writes)
        {
            IG_CORE_ASSERT(m_buffers[bid].writer_pass_idx == k_rg_invalid_id,
                           "RenderGraph: multiple writers to the same buffer are not permitted");
            m_buffers[bid].writer_pass_idx = pass_idx;
        }
        for (uint16_t bid : p.storage_buffer_writes)
        {
            IG_CORE_ASSERT(m_buffers[bid].writer_pass_idx == k_rg_invalid_id,
                           "RenderGraph: multiple writers to the same storage buffer are not permitted");
            m_buffers[bid].writer_pass_idx = pass_idx;
        }
    }

    // Phase 2: Reverse BFS from imported (root) resources → mark live passes.
    Vector<uint8_t>  pass_live(m_passes.size(), 0);
    Vector<uint16_t> tex_queue;
    Vector<uint16_t> buf_queue;
    Vector<uint16_t> waw_queue;

    for (uint16_t tid = 0; tid < (uint16_t)m_textures.size(); ++tid)
    {
        if (m_textures[tid].is_imported)
        {
            tex_queue.push_back(tid);
        }
    }

    for (uint16_t bid = 0; bid < (uint16_t)m_buffers.size(); ++bid)
    {
        if (m_buffers[bid].is_imported)
        {
            buf_queue.push_back(bid);
        }
    }

    auto try_mark_pass = [&](uint16_t writer)
    {
        if (writer == k_rg_invalid_id || pass_live[writer])
        {
            return;
        }
        pass_live[writer] = 1;
        for (uint16_t rtid : m_passes[writer]->texture_reads)
        {
            tex_queue.push_back(rtid);
        }
        for (uint16_t rbid : m_passes[writer]->buffer_reads)
        {
            buf_queue.push_back(rbid);
        }
        for (uint16_t srbid : m_passes[writer]->storage_buffer_reads)
        {
            buf_queue.push_back(srbid);
        }
        for (uint16_t srtid : m_passes[writer]->storage_texture_reads)
        {
            tex_queue.push_back(srtid);
        }
        if (m_passes[writer]->has_depth && m_passes[writer]->depth_read_only)
        {
            tex_queue.push_back(m_passes[writer]->depth_slot.texture_id);
        }
        for (uint16_t pred : waw_preds[writer])
        {
            waw_queue.push_back(pred);
        }
    };

    while (!tex_queue.empty() || !buf_queue.empty() || !waw_queue.empty())
    {
        while (!tex_queue.empty())
        {
            uint16_t tid = tex_queue.back();
            tex_queue.pop_back();
            try_mark_pass(m_textures[tid].writer_pass_idx);
        }
        while (!buf_queue.empty())
        {
            uint16_t bid = buf_queue.back();
            buf_queue.pop_back();
            try_mark_pass(m_buffers[bid].writer_pass_idx);
        }
        while (!waw_queue.empty())
        {
            uint16_t pred = waw_queue.back();
            waw_queue.pop_back();
            try_mark_pass(pred);
        }
    }

    // Phase 3: Stamp is_culled.
    for (size_t i = 0; i < m_passes.size(); ++i)
    {
        m_passes[i]->is_culled = !pass_live[i];
    }

#if defined(IG_DEBUG)
    for (uint16_t i = 0; i < (uint16_t)m_passes.size(); ++i)
    {
        if (m_passes[i]->is_culled)
        {
            continue;
        }
        for (uint16_t rtid : m_passes[i]->texture_reads)
        {
            uint16_t w = m_textures[rtid].writer_pass_idx;
            IG_CORE_ASSERT(m_textures[rtid].is_imported || w == k_rg_invalid_id || w < i,
                           "RenderGraph: pass reads a texture whose producer is declared after it");
        }
        for (uint16_t rbid : m_passes[i]->buffer_reads)
        {
            uint16_t w = m_buffers[rbid].writer_pass_idx;
            IG_CORE_ASSERT(m_buffers[rbid].is_imported || w == k_rg_invalid_id || w < i,
                           "RenderGraph: pass reads a buffer whose producer is declared after it");
        }
        for (uint16_t srbid : m_passes[i]->storage_buffer_reads)
        {
            uint16_t w = m_buffers[srbid].writer_pass_idx;
            IG_CORE_ASSERT(m_buffers[srbid].is_imported || w == k_rg_invalid_id || w < i,
                           "RenderGraph: compute pass reads a storage buffer whose producer is declared after it");
        }
        for (uint16_t srtid : m_passes[i]->storage_texture_reads)
        {
            uint16_t w = m_textures[srtid].writer_pass_idx;
            IG_CORE_ASSERT(m_textures[srtid].is_imported || w == k_rg_invalid_id || w < i,
                           "RenderGraph: compute pass reads a storage texture whose producer is declared after it");
        }
    }
#endif

    // Phase 4: Compute [first_used_pass, last_used_pass] for all live virtual resources.
    for (uint16_t i = 0; i < (uint16_t)m_passes.size(); ++i)
    {
        if (m_passes[i]->is_culled)
        {
            continue;
        }
        const RGPassBase& p = *m_passes[i];

        auto update_tex = [&](uint16_t tid, bool is_write)
        {
            if (tid == k_rg_invalid_id)
            {
                return;
            }
            RGInternal::VirtualTexture& vt = m_textures[tid];
            if (is_write && (vt.first_used_pass == k_rg_invalid_id || i < vt.first_used_pass))
            {
                vt.first_used_pass = i;
            }
            if (vt.last_used_pass == k_rg_invalid_id || i > vt.last_used_pass)
            {
                vt.last_used_pass = i;
            }
        };

        auto update_buf = [&](uint16_t bid, bool is_write)
        {
            if (bid == k_rg_invalid_id)
            {
                return;
            }
            RGInternal::VirtualBuffer& vb = m_buffers[bid];
            if (is_write && (vb.first_used_pass == k_rg_invalid_id || i < vb.first_used_pass))
            {
                vb.first_used_pass = i;
            }
            if (vb.last_used_pass == k_rg_invalid_id || i > vb.last_used_pass)
            {
                vb.last_used_pass = i;
            }
        };

        for (uint32_t s = 0; s < p.num_color_slots; ++s)
        {
            update_tex(p.color_slots[s].texture_id, true);
        }
        if (p.has_depth)
        {
            update_tex(p.depth_slot.texture_id, !p.depth_read_only);
        }
        for (uint16_t rtid : p.texture_reads)
        {
            update_tex(rtid, false);
        }
        for (uint16_t twid : p.texture_writes)
        {
            update_tex(twid, true);
        }
        for (uint16_t rbid : p.buffer_reads)
        {
            update_buf(rbid, false);
        }
        for (uint16_t wbid : p.buffer_writes)
        {
            update_buf(wbid, true);
        }
        for (uint16_t srbid : p.storage_buffer_reads)
        {
            update_buf(srbid, false);
        }
        for (uint16_t swbid : p.storage_buffer_writes)
        {
            update_buf(swbid, true);
        }
        for (uint16_t srtid : p.storage_texture_reads)
        {
            update_tex(srtid, false);
        }
        for (uint16_t stwid : p.storage_texture_writes)
        {
            update_tex(stwid, true);
        }
    }

    // Phase 5: Pool-based physical allocation — alias where lifetimes don't overlap.
    for (size_t i = 0; i < m_textures.size(); ++i)
    {
        RGInternal::VirtualTexture& vt = m_textures[i];
        if (vt.is_imported || vt.physical)
        {
            continue;
        }

        uint16_t writer = vt.writer_pass_idx;
        if (writer == k_rg_invalid_id || m_passes[writer]->is_culled)
        {
            continue;
        }

        GRITexture2DDesc desc;
        desc.width          = vt.desc.width;
        desc.height         = vt.desc.height;
        desc.format         = vt.desc.format;
        desc.num_mip_levels = vt.desc.num_mip_levels;
        vt.physical         = m_pool.acquire(desc, vt.first_used_pass, vt.last_used_pass, m_frame_index);
    }

    for (size_t i = 0; i < m_buffers.size(); ++i)
    {
        RGInternal::VirtualBuffer& vb = m_buffers[i];
        if (vb.is_imported || vb.physical)
        {
            continue;
        }

        uint16_t writer = vb.writer_pass_idx;
        if (writer == k_rg_invalid_id || m_passes[writer]->is_culled)
        {
            continue;
        }

        vb.allow_aliasing = !has_flag(vb.desc.usage, GRIBufferUsage::IndirectBuffer);

        GRIBufferDesc desc;
        desc.size  = vb.desc.size;
        desc.usage = vb.desc.usage;
        vb.physical =
            m_pool.acquire_buffer(desc, vb.first_used_pass, vb.last_used_pass, m_frame_index, vb.allow_aliasing);
    }

    m_resolved_tex.resize(m_textures.size());
    for (uint16_t i = 0; i < (uint16_t)m_textures.size(); ++i)
    {
        m_resolved_tex[i] = m_textures[i].physical;
    }

    m_resolved_buf.resize(m_buffers.size());
    for (uint16_t i = 0; i < (uint16_t)m_buffers.size(); ++i)
    {
        m_resolved_buf[i] = m_buffers[i].physical;
    }

    // Phase 6: Kahn's BFS topological sort over non-culled passes.
    {
        Vector<uint32_t>         in_degree(n, 0);
        Vector<Vector<uint16_t>> successors(n);
        uint16_t                 live_count = 0;

        for (uint16_t i = 0; i < n; ++i)
        {
            if (m_passes[i]->is_culled)
            {
                continue;
            }
            ++live_count;

            auto add_edge = [&](uint16_t writer)
            {
                if (writer == k_rg_invalid_id || m_passes[writer]->is_culled || writer == i)
                {
                    return;
                }
                for (uint16_t s : successors[writer])
                {
                    if (s == i)
                    {
                        return;
                    }
                }
                successors[writer].push_back(i);
                ++in_degree[i];
            };

            for (uint16_t rtid : m_passes[i]->texture_reads)
            {
                add_edge(m_textures[rtid].writer_pass_idx);
            }
            for (uint16_t rbid : m_passes[i]->buffer_reads)
            {
                add_edge(m_buffers[rbid].writer_pass_idx);
            }
            for (uint16_t srbid : m_passes[i]->storage_buffer_reads)
            {
                add_edge(m_buffers[srbid].writer_pass_idx);
            }
            for (uint16_t srtid : m_passes[i]->storage_texture_reads)
            {
                add_edge(m_textures[srtid].writer_pass_idx);
            }
            for (uint16_t pred : waw_preds[i])
            {
                add_edge(pred);
            }
            if (m_passes[i]->has_depth && m_passes[i]->depth_read_only)
            {
                add_edge(m_textures[m_passes[i]->depth_slot.texture_id].writer_pass_idx);
            }
        }

        Deque<uint16_t> q;
        for (uint16_t i = 0; i < n; ++i)
        {
            if (!m_passes[i]->is_culled && in_degree[i] == 0)
            {
                q.push_back(i);
            }
        }

        m_sorted_passes.reserve(live_count);
        while (!q.empty())
        {
            uint16_t idx = q.front();
            q.pop_front();
            m_sorted_passes.push_back(idx);
            for (uint16_t succ : successors[idx])
            {
                if (--in_degree[succ] == 0)
                {
                    q.push_back(succ);
                }
            }
        }

        IG_CORE_ASSERT(m_sorted_passes.size() == live_count, "RenderGraph: dependency cycle detected");
    }

    // Phase 3.5: Hazard injection.
    // Walk adjacent non-culled pass pairs and insert GRIBarriers for any shared physical
    // resource that transitions between incompatible access modes (RAW, WAR, WAW).
    {
        const size_t sorted_count = m_sorted_passes.size();
        m_sorted_barriers.assign(sorted_count, Vector<RGBarrier>{});

        // Reverse map: pass declaration index → position in m_sorted_passes.
        Vector<uint16_t> sorted_pos(n, k_rg_invalid_id);
        for (uint16_t sp = 0; sp < (uint16_t)sorted_count; ++sp)
        {
            sorted_pos[m_sorted_passes[sp]] = sp;
        }

        // Helper: push a barrier into slot [si], deduplicating on physical resource.
        auto push_barrier = [&](size_t si, GRIResource* phys, GRIAccessFlags old_a, GRIAccessFlags new_a)
        {
            for (const RGBarrier& bx : m_sorted_barriers[si])
            {
                if (bx.resource == phys)
                {
                    return;
                }
            }
            m_sorted_barriers[si].push_back({phys, old_a, new_a});
        };

        // Phase 3.5a: Direct hazards between adjacent sorted pass pairs.
        for (size_t si = 1; si < sorted_count; ++si)
        {
            const uint16_t    a_idx = m_sorted_passes[si - 1];
            const uint16_t    b_idx = m_sorted_passes[si];
            const RGPassBase& passA = *m_passes[a_idx];
            const RGPassBase& passB = *m_passes[b_idx];

            auto check_buf = [&](uint16_t bid)
            {
                if (bid == k_rg_invalid_id)
                {
                    return;
                }
                GRIBuffer* phys = m_resolved_buf[bid];
                if (!phys)
                {
                    return;
                }
                const GRIBufferUsage usage = m_buffers[bid].desc.usage;
                const GRIAccessFlags acc_a = get_buffer_access(passA, bid, usage);
                const GRIAccessFlags acc_b = get_buffer_access(passB, bid, usage);
                if (!is_hazard(acc_a, acc_b))
                {
                    return;
                }
                push_barrier(si, phys, acc_a, acc_b);
            };

            auto check_tex = [&](uint16_t tid)
            {
                if (tid == k_rg_invalid_id)
                {
                    return;
                }
                GRITexture2D* phys = m_resolved_tex[tid];
                if (!phys)
                {
                    return;
                }
                const GRIAccessFlags acc_a = get_texture_access(passA, tid);
                const GRIAccessFlags acc_b = get_texture_access(passB, tid);
                if (!is_hazard(acc_a, acc_b))
                {
                    return;
                }
                push_barrier(si, phys, acc_a, acc_b);
            };

            for (uint16_t bid : passA.storage_buffer_reads)
            {
                check_buf(bid);
            }
            for (uint16_t bid : passA.storage_buffer_writes)
            {
                check_buf(bid);
            }
            for (uint16_t bid : passA.buffer_reads)
            {
                check_buf(bid);
            }
            for (uint16_t bid : passA.buffer_writes)
            {
                check_buf(bid);
            }

            for (uint16_t tid : passA.storage_texture_reads)
            {
                check_tex(tid);
            }
            for (uint16_t tid : passA.storage_texture_writes)
            {
                check_tex(tid);
            }
            for (uint16_t tid : passA.texture_reads)
            {
                check_tex(tid);
            }
            for (uint16_t tid : passA.texture_writes)
            {
                check_tex(tid);
            }
            for (uint32_t s = 0; s < passA.num_color_slots; ++s)
            {
                check_tex(passA.color_slots[s].texture_id);
            }
            if (passA.has_depth)
            {
                check_tex(passA.depth_slot.texture_id);
            }
        }

        // Phase 3.5b: Aliased resource transition safety.
        // Two transient virtual buffers that share a physical allocation (non-overlapping
        // lifetimes) require an explicit barrier so the second owner's encoder cannot
        // observe stale writes from the first owner's encoder.
        {
            struct AliasEntry
            {
                uint16_t vb_id;
                uint16_t first_pass;
                uint16_t last_pass;
            };
            UnorderedMap<GRIBuffer*, Vector<AliasEntry>> alias_map;

            for (uint16_t i = 0; i < (uint16_t)m_buffers.size(); ++i)
            {
                const RGInternal::VirtualBuffer& vb = m_buffers[i];
                if (!vb.physical || vb.is_imported || !vb.allow_aliasing)
                {
                    continue;
                }
                if (vb.first_used_pass == k_rg_invalid_id)
                {
                    continue;
                }
                alias_map[vb.physical].push_back({i, vb.first_used_pass, vb.last_used_pass});
            }

            for (auto& [phys, entries] : alias_map)
            {
                if (entries.size() < 2)
                {
                    continue;
                }
                std::sort(entries.begin(), entries.end(),
                          [](const AliasEntry& x, const AliasEntry& y) { return x.first_pass < y.first_pass; });

                for (size_t e = 0; e + 1 < entries.size(); ++e)
                {
                    const AliasEntry& ea = entries[e];
                    const AliasEntry& eb = entries[e + 1];
                    IG_CORE_ASSERT(ea.last_pass < eb.first_pass,
                                   "RenderGraph: aliased buffers have overlapping lifetimes");

                    const GRIBufferUsage usage_a = m_buffers[ea.vb_id].desc.usage;
                    const GRIBufferUsage usage_b = m_buffers[eb.vb_id].desc.usage;
                    const GRIAccessFlags acc_a   = get_buffer_access(*m_passes[ea.last_pass], ea.vb_id, usage_a);
                    const GRIAccessFlags acc_b   = get_buffer_access(*m_passes[eb.first_pass], eb.vb_id, usage_b);
                    if (acc_a == GRIAccessFlags::None || acc_b == GRIAccessFlags::None)
                    {
                        continue;
                    }

                    const uint16_t sp_b = sorted_pos[eb.first_pass];
                    if (sp_b == k_rg_invalid_id)
                    {
                        continue;
                    }
                    push_barrier(sp_b, phys, acc_a, acc_b);
                }
            }
        }
    }

    m_topo_cache.last_fingerprint = m_current_fingerprint;
}

GRIRenderPassInfo RenderGraph::build_pass_info(const RGPassBase& pass) const
{
    GRIRenderPassInfo info;
    info.num_explicit_colour_targets = pass.num_color_slots;

    for (uint32_t s = 0; s < pass.num_color_slots; ++s)
    {
        GRIRenderPassInfo::ColourEntry& entry = info.colour_targets[s];

        const RGInternal::AttachmentSlot& slot = pass.color_slots[s];
        if (slot.texture_id == k_rg_invalid_id)
        {
            entry.render_target = nullptr;
            continue;
        }

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
