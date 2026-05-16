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
		struct ColourEntry
		{
			GRITexture2D* render_target = nullptr;
			uint32_t mip_index = 0;
			uint32_t array_slice_index = -1;
		};

		struct DepthEntry
		{
			GRITexture2D* depth_stencil_target = nullptr;
		};

		ColourEntry colour_targets[max_simultaneous_render_targets];
		DepthEntry depth_stencil_target;

		GRIRenderPassInfo() = default;
		GRIRenderPassInfo(const GRIRenderPassInfo&) = default;
		GRIRenderPassInfo& operator=(const GRIRenderPassInfo&) = default;

		GRIRenderPassInfo(GRITexture2D* colour, uint32_t mip_index = 0, uint32_t array_slice_index = -1)
		{
			colour_targets[0].render_target = colour;
			colour_targets[0].mip_index = mip_index;
			colour_targets[0].array_slice_index = array_slice_index;
		}

		GRIRenderPassInfo(GRITexture2D* colour, uint32_t mip_index = 0, uint32_t array_slice_index = -1, GRITexture2D* depth_stencil = nullptr)
		{
			colour_targets[0].render_target = colour;
			colour_targets[0].mip_index = mip_index;
			colour_targets[0].array_slice_index = array_slice_index;

			depth_stencil_target.depth_stencil_target = depth_stencil;
		}

		inline int32_t get_num_colour_targets() const 
		{ 
			int32_t num_targets = 0;
			for (; num_targets < max_simultaneous_render_targets; num_targets++)
			{
				const ColourEntry& entry = colour_targets[num_targets];
				if (!entry.render_target)
				{
					break;
				}
			}
			return num_targets;
		}
	};

	struct GRITexture2DDesc
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t num_mip_levels = 1;
	};

	class GRITexture2D
	{
	public:
		virtual ~GRITexture2D() = default;
		virtual uint32_t get_width() const = 0;
		virtual uint32_t get_height() const = 0;
		virtual uint32_t get_mip_count() const = 0;

		virtual void* get_native_handle() const = 0;
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