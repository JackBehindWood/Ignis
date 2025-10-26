#include "Ignis/Rendering/GRI/GRI.h"

namespace MTL { class Device; }

namespace Ignis
{
    class MetalGRI : public GRI
    {
    private:
        MTL::Device* m_device;
    public:
        MetalGRI();
        ~MetalGRI() = default;

        void init() override;
        void shutdown() override;

        GRIViewportPtr create_viewport(uint32_t width, uint32_t height) override;

        inline GRIRenderAPI get_api() const override
        {
            return GRIRenderAPI::Metal;
        }

        inline MTL::Device* get_device() const
        {
            return m_device;
        }
    };
} // namespace Ignis