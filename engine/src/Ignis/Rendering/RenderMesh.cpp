#include "igpch.h"
#include "RenderMesh.h"

#include "RenderSystem.h"

namespace Ignis
{

SharedPtr<RenderMesh> RenderMesh::create(const void* vertex_data, uint32_t vertex_data_size, const uint32_t* index_data,
                                         uint32_t index_count, GRIIndexFormat index_format, Math::Vec3f bounds_center,
                                         float bounds_radius, MeshSlot slot)
{
    GRI*          gri = RenderSystem::get_gri();
    GRIBufferDesc vb_desc;
    vb_desc.size    = vertex_data_size;
    vb_desc.usage   = GRIBufferUsage::VertexBuffer;
    GRIBufferPtr vb = gri->create_buffer(vb_desc, vertex_data);

    GRIBufferDesc ib_desc;
    ib_desc.size    = index_count * sizeof(uint32_t);
    ib_desc.usage   = GRIBufferUsage::IndexBuffer;
    GRIBufferPtr ib = gri->create_buffer(ib_desc, index_data);

    if (slot.index_count == 0)
    {
        slot.index_count = index_count;
    }

    return create_shared<RenderMesh>(std::move(vb), std::move(ib), index_count, index_format, bounds_center,
                                     bounds_radius, slot);
}

} // namespace Ignis
