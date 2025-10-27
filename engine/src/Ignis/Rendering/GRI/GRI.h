#pragma once

#include "GRIResource.h"

namespace Ignis
{    
    enum class GRIRenderAPI
    {
        None = 0,
        OpenGL = 1,
        Vulkan = 2,
        DirectX12 = 3,
        Metal = 4
    };
    class GRI
    {
    public:
        static GRI* create(GRIRenderAPI api);

        virtual ~GRI() = default;

        virtual void init() = 0;
        virtual void shutdown() = 0;

        virtual GRIRenderAPI get_api() const = 0;

        virtual GRIViewportPtr create_viewport(const GRIViewportDesc& desc) = 0;
        virtual void resize_viewport(GRIViewport* viewport, uint32_t width, uint32_t height) = 0;
    };
    
} // namespace Ignis


