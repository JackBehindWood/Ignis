#pragma once

#include "GRIResource.h"

namespace Ignis
{    
    class GRI
    {
    public:
        static GRI* create(GRIRenderAPI api);

        virtual ~GRI() = default;

        virtual void init() = 0;
        virtual void shutdown() = 0;

        virtual GRIRenderAPI get_api() const = 0;
        virtual GRICommandContext* get_context() = 0;

        virtual GRIViewportPtr create_viewport(const GRIViewportDesc& desc) = 0;
        virtual void resize_viewport(GRIViewport* viewport, uint32_t width, uint32_t height) = 0;
    };
    
} // namespace Ignis


