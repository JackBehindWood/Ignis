#include "Ignis/Rendering/GRI/GRI.h"

namespace Ignis
{
    class MetalGRI : public GRI
    {
    public:
        MetalGRI();
        ~MetalGRI();

        void init() override;
        void shutdown() override;

        GRIViewportPtr create_viewport(uint32_t width, uint32_t height) override;

        inline GRIRenderAPI get_api() const override
        {
            return GRIRenderAPI::Metal;
        }
    };
} // namespace Ignis