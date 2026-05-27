#include "MetalDevice.h"
#include "MetalResource.h"

#include <Ignis/Rendering/GRI/GRIResource.h>
#include <Metal/Metal.hpp>

namespace Ignis
{
class MetalStateCache
{
private:
    MetalDevice&               m_device;
    MTL::RenderPassDescriptor* m_render_pass_descriptor;

    // Index buffer binding
    MTL::Buffer*   m_index_buffer;
    MTL::IndexType m_index_type;
    NS::UInteger   m_index_buffer_offset;

    // Current primitive topology (sourced from pipeline state)
    MTL::PrimitiveType m_primitive_type;

    // Render targets set by begin_drawing_viewport, consumed by begin_render_pass
    GRIRenderPassInfo m_pending_pass_info;

public:
    MetalStateCache(MetalDevice& device);
    ~MetalStateCache();

    void reset();

    // Render pass
    void set_pending_pass_info(const GRIRenderPassInfo& info);
    void set_render_pass_info(const GRIRenderPassInfo& info);

    inline const GRIRenderPassInfo& get_pending_pass_info() const
    {
        return m_pending_pass_info;
    }
    inline MTL::RenderPassDescriptor* get_render_pass_descriptor()
    {
        return m_render_pass_descriptor;
    }

    // Index buffer
    void set_index_buffer(MTL::Buffer* buffer, MTL::IndexType type, NS::UInteger offset);

    inline MTL::Buffer* get_index_buffer() const
    {
        return m_index_buffer;
    }
    inline MTL::IndexType get_index_type() const
    {
        return m_index_type;
    }
    inline NS::UInteger get_index_buffer_offset() const
    {
        return m_index_buffer_offset;
    }

    // Primitive type
    void                      set_primitive_type(MTL::PrimitiveType type);
    inline MTL::PrimitiveType get_primitive_type() const
    {
        return m_primitive_type;
    }
};

class MetalRenderPassDescriptorPool
{
private:
    Vector<MTL::RenderPassDescriptor*> m_descriptors;

public:
    MetalRenderPassDescriptorPool()
    {
    }
    ~MetalRenderPassDescriptorPool();

    MTL::RenderPassDescriptor* create_descriptor();
    void                       release_descriptor(MTL::RenderPassDescriptor* descriptor);

    static MetalRenderPassDescriptorPool& get()
    {
        static MetalRenderPassDescriptorPool pool;
        return pool;
    }
};
} // namespace Ignis