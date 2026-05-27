#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Foundation/Memory.h"

namespace Ignis
{

class AssetMesh : public Asset
{
public:
    // Vertices now stored as generic bytes to remain format-agnostic
    AssetMesh(AssetID id, const Vector<uint8_t>& vertices, const Vector<uint32_t>& indices, uint32_t vertex_stride)
        : m_vertices(vertices),
          m_indices(indices),
          m_vertex_stride(vertex_stride)
    {
        m_id = id;
    }

    const Vector<uint8_t>& get_vertices() const
    {
        return m_vertices;
    }
    const Vector<uint32_t>& get_indices() const
    {
        return m_indices;
    }
    uint32_t get_vertex_stride() const
    {
        return m_vertex_stride;
    }

    static AssetType static_type()
    {
        return AssetType::Mesh;
    }

private:
    Vector<uint8_t>  m_vertices;
    Vector<uint32_t> m_indices;
    uint32_t         m_vertex_stride = 0;
};

} // namespace Ignis