#include "igpch.h"
#include "MetalContext.h"

#include "MetalResource.h"

namespace  Ignis
{
    void MetalCommandContext::set_render_targets(uint32_t num_render_targets, const GRIRenderTargetView* render_targets, const GRIDepthRenderTargetView* depth_stencil_target)
    {
        IG_CORE_ASSERT(num_render_targets <= max_simultaneous_render_targets, "");
        MTL_AUTORELEASE_POOL;

        GRIDepthRenderTargetView depth_target;
        if (depth_stencil_target)
        {
            depth_target = *depth_stencil_target;
        }
        else
        {
            depth_target = GRIDepthRenderTargetView(nullptr);
        }

        GRIRenderTargetsInfo info(num_render_targets, render_targets, depth_target);
        set_render_targets_and_clear(info);
    }

    void MetalCommandContext::set_render_targets_and_clear(const GRIRenderTargetsInfo& info) 
    {
        GRIRenderPassInfo pass_info;
        m_state_cache.set_render_pass_info(pass_info);
    }
} // namespace  Ignes
