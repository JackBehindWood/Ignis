#include "MetalDevice.h"

#include "MetalResource.h"

namespace MTL
{
    class RenderPassDescriptor;
}

namespace Ignis
{
    class MetalStateCache
    {
    private:
        MetalDevice& m_device;
        MTL::RenderPassDescriptor* m_render_pass_descriptor;
    public:
        MetalStateCache(MetalDevice& device);
        ~MetalStateCache();

        void reset();
        void set_render_pass_info(const GRIRenderPassInfo& info);
    };

    class MetalRenderPassDescriptorPool
    {
    private:
        Vector<MTL::RenderPassDescriptor*> m_descriptors;
    public:
        MetalRenderPassDescriptorPool() {}
        ~MetalRenderPassDescriptorPool();

        MTL::RenderPassDescriptor* create_descriptor();
        void release_descriptor(MTL::RenderPassDescriptor* descriptor);

        static MetalRenderPassDescriptorPool& get()
        {
            static MetalRenderPassDescriptorPool pool;
            return pool;
        }
    };
}