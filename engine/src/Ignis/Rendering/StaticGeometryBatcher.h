#pragma once

#include "RenderMesh.h"
#include "GRI/GRIResource.h"
#include "GRI/GRIDefinitions.h"

namespace Ignis
{

class StaticGeometryBatcher
{
public:
    static StaticGeometryBatcher& get()
    {
        static StaticGeometryBatcher s_instance;
        return s_instance;
    }

    MeshSlot register_mesh(uint64_t key, const void* vertex_data, uint32_t vertex_byte_size, const uint32_t* index_data,
                           uint32_t index_count);
    void     flush_to_gpu();

    GRIBuffer* get_global_vb() const
    {
        return m_vb.get();
    }
    GRIBuffer* get_global_ib() const
    {
        return m_ib.get();
    }

private:
    StaticGeometryBatcher() = default;

    UnorderedMap<uint64_t, MeshSlot> m_slots;
    Vector<uint8_t>                  m_vertices;
    Vector<uint32_t>                 m_indices;
    GRIBufferPtr                     m_vb;
    GRIBufferPtr                     m_ib;
    bool                             m_dirty = false;
};

} // namespace Ignis
