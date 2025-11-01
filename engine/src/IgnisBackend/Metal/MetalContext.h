#pragma once

#include <Ignis/Rendering/GRI/GRIContext.h>
#include "MetalDevice.h"

#include "MetalStateCache.h"

namespace Ignis
{
    class MetalCommandContext : public GRICommandContext
    {
    private:
        MetalDevice& m_device;
        MetalStateCache m_state_cache;
    public:
        MetalCommandContext(MetalDevice& device) : m_device(device), m_state_cache(device) {};
        virtual ~MetalCommandContext() = default;

        virtual void begin_drawing_viewport(GRIViewport* viewport, GRITexture2D* render_target) override;
        virtual void end_drawing_viewport(GRIViewport* viewport) override;

        virtual void set_render_targets(uint32_t num_render_targets, const GRIRenderTargetView* render_targets, const GRIDepthRenderTargetView* depth_stencil_target) override;
        virtual void set_render_targets_and_clear(const GRIRenderTargetsInfo& info) override;
    };
} // namespace  Ignis
