#pragma Once

#include "GRIResource.h"

namespace Ignis
{
    class GRICommandContext
    {
    public:
        virtual ~GRICommandContext() = default;

        virtual void begin_drawing_viewport(GRIViewport* viewport, GRITexture2D* render_target) = 0;
        virtual void end_drawing_viewport(GRIViewport* viewport) = 0;

		virtual void set_render_targets(uint32_t num_render_targets, const GRIRenderTargetView* render_targets, const GRIDepthRenderTargetView* depth_stencil_target) = 0;
        virtual void set_render_targets_and_clear(const GRIRenderTargetsInfo& info) = 0;
    };
} // namespace Ignis
