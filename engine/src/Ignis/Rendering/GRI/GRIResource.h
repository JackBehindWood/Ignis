#pragma once

#include "GRIDefinitions.h"

namespace Ignis
{
	class GRIRenderTargetView
	{
	public:
		GRITexture2D* texture;
		uint32_t mip_index;
		uint32_t array_slice_index;
		GRIRenderTargetView() : texture(nullptr), mip_index(0), array_slice_index(-1) {}
		GRIRenderTargetView(GRITexture2D* texture) : texture(texture), mip_index(0), array_slice_index(-1) {}

		GRIRenderTargetView(GRIRenderTargetView&&) = default;
		GRIRenderTargetView(const GRIRenderTargetView&) = default;
		GRIRenderTargetView& operator=(GRIRenderTargetView&&) = default;
		GRIRenderTargetView& operator=(const GRIRenderTargetView&) = default;
	};

	class GRIDepthRenderTargetView
	{
	public:
		GRITexture2D* texture;
		GRIDepthRenderTargetView() : texture(nullptr) {}
		GRIDepthRenderTargetView(GRITexture2D* texture) : texture(texture) {}

		GRIDepthRenderTargetView(GRIDepthRenderTargetView&&) = default;
		GRIDepthRenderTargetView(const GRIDepthRenderTargetView&) = default;
		GRIDepthRenderTargetView& operator=(GRIDepthRenderTargetView&&) = default;
		GRIDepthRenderTargetView& operator=(const GRIDepthRenderTargetView&) = default;
	};

	class GRIRenderTargetsInfo
	{
	public:
		uint32_t num_targets;
		GRIRenderTargetView colour_targets[max_simultaneous_render_targets];
		GRIDepthRenderTargetView depth_stencil_target;

		GRIRenderTargetsInfo() : num_targets(0), depth_stencil_target() {}
		GRIRenderTargetsInfo(uint32_t num_targets, const GRIRenderTargetView* render_target, const GRIDepthRenderTargetView& depth_stencil_target) : num_targets(num_targets), depth_stencil_target(depth_stencil_target) 
		{
			for (uint32_t i = 0; i < num_targets; i++)
			{
				colour_targets[i] = render_target[i];
			}
		}
	};

	class GRIRenderPassInfo
	{
	public:
		GRIRenderPassInfo() = default;
		GRIRenderPassInfo(const GRIRenderPassInfo&) = default;
		GRIRenderPassInfo& operator=(const GRIRenderPassInfo&) = default;
	};

    struct GRIViewportDesc
    {
        uint32_t width = 800;
        uint32_t height = 600;

        const char* title = "GRIViewport";
    };

    class GRIViewport
    {
    public:
        virtual ~GRIViewport() = default;
        virtual uint32_t get_width() const = 0;
        virtual uint32_t get_height() const = 0;

        virtual void* get_native_handle() const = 0;
    };
}