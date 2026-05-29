#pragma once

#include "Ignis/Rendering/GRI/GRIResource.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"
#include "Ignis/Math/Vec3.h"

namespace Ignis
{

struct MeshSlot
{
    uint32_t first_index = 0;
    int32_t  base_vertex = 0;
    uint32_t index_count = 0;
};

class RenderMesh : public RefCounted
{
public:
    RenderMesh(GRIBufferPtr vertex_buffer, GRIBufferPtr index_buffer, uint32_t index_count, GRIIndexFormat index_format,
               Math::Vec3f bounds_center, float bounds_radius, MeshSlot slot)
        : m_vertex_buffer(std::move(vertex_buffer)),
          m_index_buffer(std::move(index_buffer)),
          m_index_count(index_count),
          m_index_format(index_format),
          m_bounds_center(bounds_center),
          m_bounds_radius(bounds_radius),
          m_slot(slot)
    {
    }

    static SharedPtr<RenderMesh> create(const void* vertex_data, uint32_t vertex_data_size, const uint32_t* index_data,
                                        uint32_t index_count, GRIIndexFormat index_format = GRIIndexFormat::Uint32,
                                        Math::Vec3f bounds_center = {}, float bounds_radius = 1.0e30f,
                                        MeshSlot slot = {});

    static SharedPtr<RenderMesh> create_batched(Math::Vec3f bounds_center, float bounds_radius, MeshSlot slot);

    GRIBuffer* get_vertex_buffer() const
    {
        return m_vertex_buffer.get();
    }
    GRIBuffer* get_index_buffer() const
    {
        return m_index_buffer.get();
    }
    uint32_t get_index_count() const
    {
        return m_index_count;
    }
    GRIIndexFormat get_index_format() const
    {
        return m_index_format;
    }
    Math::Vec3f get_bounds_center() const
    {
        return m_bounds_center;
    }
    float get_bounds_radius() const
    {
        return m_bounds_radius;
    }
    MeshSlot get_mesh_slot() const
    {
        return m_slot;
    }

private:
    GRIBufferPtr   m_vertex_buffer;
    GRIBufferPtr   m_index_buffer;
    uint32_t       m_index_count;
    GRIIndexFormat m_index_format;
    Math::Vec3f    m_bounds_center;
    float          m_bounds_radius;
    MeshSlot       m_slot;
};

} // namespace Ignis
