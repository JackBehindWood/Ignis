#include "MetalDevice.h"
#include "Ignis/Rendering/GRI/GRIResource.h"

namespace Ignis
{
    class MetalStateCache
    {
    private:
        MetalDevice& m_device;
    public:
        MetalStateCache(MetalDevice& device);
        ~MetalStateCache();

        void reset();

        void set_render_pass_info(const GRIRenderPassInfo& info);
    };
}