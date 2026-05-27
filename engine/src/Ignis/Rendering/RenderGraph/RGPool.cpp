#include "igpch.h"
#include "RGPool.h"
#include <Ignis/Rendering/GRI/GRI.h>
#include <Ignis/Rendering/RenderSystem.h>

namespace Ignis
{

GRITexture2D* RenderGraphResourcePool::acquire(const GRITexture2DDesc& desc, uint16_t first_used, uint16_t last_used,
                                               uint32_t current_frame)
{
    for (TextureEntry& e : m_texture_entries)
    {
        if (e.desc.width != desc.width || e.desc.height != desc.height || e.desc.format != desc.format ||
            e.desc.num_mip_levels != desc.num_mip_levels)
        {
            continue;
        }

        bool overlaps = false;
        for (const Interval& iv : e.committed)
        {
            if (first_used <= iv.last && iv.first <= last_used)
            {
                overlaps = true;
                break;
            }
        }

        if (!overlaps)
        {
            e.committed.push_back({first_used, last_used});
            e.last_frame_used = current_frame;
            return e.resource.get();
        }
    }

    // Note: only acquiring the gri is allowed from the render system!
    GRI* gri = RenderSystem::get_gri();

    TextureEntry& entry   = m_texture_entries.emplace_back();
    entry.desc            = desc;
    entry.last_frame_used = current_frame;
    entry.resource        = gri->create_texture2d(desc);
    entry.committed.push_back({first_used, last_used});
    return entry.resource.get();
}

GRIBuffer* RenderGraphResourcePool::acquire_buffer(const GRIBufferDesc& desc, uint16_t first_used, uint16_t last_used,
                                                   uint32_t current_frame)
{
    for (BufferEntry& e : m_buffer_entries)
    {
        if (e.desc.size != desc.size || e.desc.usage != desc.usage)
        {
            continue;
        }

        bool overlaps = false;
        for (const Interval& iv : e.committed)
        {
            if (first_used <= iv.last && iv.first <= last_used)
            {
                overlaps = true;
                break;
            }
        }

        if (!overlaps)
        {
            e.committed.push_back({first_used, last_used});
            e.last_frame_used = current_frame;
            return e.resource.get();
        }
    }

    // Note: only acquiring the gri is allowed from the render system!
    GRI* gri = RenderSystem::get_gri();

    BufferEntry& entry    = m_buffer_entries.emplace_back();
    entry.desc            = desc;
    entry.last_frame_used = current_frame;
    entry.resource        = gri->create_buffer(desc);
    entry.committed.push_back({first_used, last_used});
    return entry.resource.get();
}

void RenderGraphResourcePool::begin_frame(uint32_t current_frame)
{
    auto evict = [&](auto& entries)
    {
        size_t write = 0;
        for (size_t i = 0; i < entries.size(); ++i)
        {
            if (current_frame <= entries[i].last_frame_used + k_eviction_age)
            {
                entries[write++] = std::move(entries[i]);
            }
        }
        entries.resize(write);
        for (auto& e : entries)
        {
            e.committed.clear();
        }
    };

    evict(m_texture_entries);
    evict(m_buffer_entries);
}

} // namespace Ignis
