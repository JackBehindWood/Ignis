#pragma once

#include <Ignis/Rendering/GRI/GRIContext.h>

namespace Ignis
{
    class MetalCommandContext : public GRICommandContext
    {
    public:
        virtual ~MetalCommandContext() = default;

        virtual void begin_drawing_viewport(GRIViewport* viewport) override {}
        virtual void end_drawing_viewport(GRIViewport* viewport) override {}
    };
} // namespace  Ignis
