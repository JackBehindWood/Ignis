#pragma once

#include "Ignis/Rendering/GRI/GRIResource.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"

namespace Ignis
{
    //TODO: this is currently just a wrapper around vertex/index buffers, but we may want to add more mesh-related data here in the future (e.g. submesh info, bounding volumes, etc.)
    //TODO: Check if we really want this to be ref counted and used with SharedPtr!
    class RenderMesh : public RefCounted
    {
    public:
        RenderMesh(GRIBufferPtr vertex_buffer, GRIBufferPtr index_buffer,
                   uint32_t index_count, GRIIndexFormat index_format)
            : m_vertex_buffer(std::move(vertex_buffer))
            , m_index_buffer(std::move(index_buffer))
            , m_index_count(index_count)
            , m_index_format(index_format)
        {}

        static SharedPtr<RenderMesh> create(
            const void* vertex_data, uint32_t vertex_data_size,
            const uint32_t* index_data, uint32_t index_count,
            GRIIndexFormat index_format = GRIIndexFormat::Uint32);

        GRIBuffer*                  get_vertex_buffer()      const { return m_vertex_buffer.get(); }
        GRIBuffer*                  get_index_buffer()       const { return m_index_buffer.get(); }
        uint32_t                    get_index_count()        const { return m_index_count; }
        GRIIndexFormat              get_index_format()       const { return m_index_format; }

    private:
        GRIBufferPtr         m_vertex_buffer;
        GRIBufferPtr         m_index_buffer;
        uint32_t             m_index_count;
        GRIIndexFormat       m_index_format;
    };

} // namespace Ignis
