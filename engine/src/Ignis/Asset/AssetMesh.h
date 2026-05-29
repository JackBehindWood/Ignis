#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Foundation/Memory.h"
#include "Ignis/Math/Vec3.h"

namespace Ignis
{

class AssetMesh : public Asset
{
public:
    AssetMesh(AssetID id, const Vector<uint8_t>& vertices, const Vector<uint32_t>& indices, uint32_t vertex_stride)
        : m_vertices(vertices),
          m_indices(indices),
          m_vertex_stride(vertex_stride),
          m_first_index(0),
          m_base_vertex(0),
          m_bounds_center{},
          m_bounds_radius(1.0e30f)
    {
        m_id = id;
    }

    void set_bounds(Math::Vec3f center, float radius)
    {
        m_bounds_center = center;
        m_bounds_radius = radius;
    }

    void set_batch_slot(uint32_t first_index, int32_t base_vertex)
    {
        m_first_index = first_index;
        m_base_vertex = base_vertex;
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

    Math::Vec3f get_bounds_center() const
    {
        return m_bounds_center;
    }
    float get_bounds_radius() const
    {
        return m_bounds_radius;
    }
    uint32_t get_first_index() const
    {
        return m_first_index;
    }
    int32_t get_base_vertex() const
    {
        return m_base_vertex;
    }

    static AssetType static_type()
    {
        return AssetType::Mesh;
    }

private:
    Vector<uint8_t>  m_vertices;
    Vector<uint32_t> m_indices;
    uint32_t         m_vertex_stride = 0;
    uint32_t         m_first_index   = 0;
    int32_t          m_base_vertex   = 0;
    Math::Vec3f      m_bounds_center;
    float            m_bounds_radius = 1.0e30f;
};

} // namespace Ignis
