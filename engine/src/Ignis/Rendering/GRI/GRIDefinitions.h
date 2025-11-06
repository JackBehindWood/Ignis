#pragma once

namespace Ignis
{
    class GRI;
    class GRICommandContext;
    class GRICommandListBase;
    class GRIResource;
    class GRIViewport;
    class GRITexture2D;

    using GRIViewportPtr = UniquePtr<GRIViewport>;
    using GRITexture2DPtr = SharedPtr<GRITexture2D>;

    enum
	{
		max_simultaneous_render_targets = 8,
	};


    enum class GRIResourceType
    {
        Texture,
        Buffer,
        Shader,
        Pipeline,
        RenderTarget,
        Sampler,
        Unknown
    };

    enum class GRIRenderAPI
    {
        None = 0,
        OpenGL = 1,
        Vulkan = 2,
        DirectX12 = 3,
        Metal = 4
    };
} // namespace Ignis
