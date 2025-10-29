#include "Ignis/Rendering/GRI/GRI.h"
#include "MetalContext.h"

#include "MetalResource.h"

namespace Ignis
{
    class MetalGRI : public GRI
    {
    private:
        MetalDevice* m_device;
        MetalCommandContext m_context;
    public:
        MetalGRI();
        ~MetalGRI() = default;

        void init() override;
        void shutdown() override;

        GRIViewportPtr create_viewport(const GRIViewportDesc& desc) override;
        void resize_viewport(GRIViewport* viewport, uint32_t width, uint32_t height) override;

        inline GRIRenderAPI get_api() const override
        {
            return GRIRenderAPI::Metal;
        }

        inline MetalDevice* get_device() const
        {
            return m_device;
        }

        inline GRICommandContext* get_context() override
        {
            return &m_context;
        }

        template <typename T>
		static inline typename MetalResourceTraits<T>::ConcreteType* resource_cast(T* resource)
		{
			return static_cast<typename MetalResourceTraits<T>::ConcreteType*>(resource);
		}
    };
} // namespace Ignis