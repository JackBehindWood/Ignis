#include "igpch.h"
#include "StaticGeometryBatcher.h"
#include "RenderSystem.h"

namespace Ignis
{

MeshSlot StaticGeometryBatcher::register_mesh(uint64_t key, const void* vertex_data, uint32_t vertex_byte_size,
                                              const uint32_t* index_data, uint32_t index_count)
{
    auto it = m_slots.find(key);
    if (it != m_slots.end())
    {
        return it->second;
    }

    MeshSlot slot;
    slot.first_index = static_cast<uint32_t>(m_indices.size());
    slot.base_vertex = static_cast<int32_t>(m_vertices.size() / 32u);
    slot.index_count = index_count;

    const uint8_t* vb = static_cast<const uint8_t*>(vertex_data);
    m_vertices.insert(m_vertices.end(), vb, vb + vertex_byte_size);
    m_indices.insert(m_indices.end(), index_data, index_data + index_count);

    m_slots[key] = slot;
    m_dirty      = true;
    return slot;
}

void StaticGeometryBatcher::flush_to_gpu()
{
    if (!m_dirty || m_vertices.empty())
    {
        return;
    }

    GRI* gri = RenderSystem::get_gri();

    GRIBufferDesc vb_desc;
    vb_desc.size  = static_cast<uint32_t>(m_vertices.size());
    vb_desc.usage = GRIBufferUsage::VertexBuffer;
    m_vb          = gri->create_buffer(vb_desc, m_vertices.data());

    GRIBufferDesc ib_desc;
    ib_desc.size  = static_cast<uint32_t>(m_indices.size() * sizeof(uint32_t));
    ib_desc.usage = GRIBufferUsage::IndexBuffer;
    m_ib          = gri->create_buffer(ib_desc, m_indices.data());

    m_dirty = false;
}

} // namespace Ignis
