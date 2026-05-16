#pragma once

namespace Ignis
{
    class GRI;
    class GRICommandContext;
    class GRICommandListBase;
    class GRIResource;
    class GRIViewport;
    class GRITexture2D;
    class GRIVertexShader;
    class GRIPixelShader;
    class GRIPipelineState;
    class GRIVertexDeclaration;
    class GRIBuffer;

    template<typename T>
    using GRIResourcePtr = SharedPtr<T>;

    using GRIViewportPtr       = UniquePtr<GRIViewport>;
    using GRITexture2DPtr      = GRIResourcePtr<GRITexture2D>;
    using GRIVertexShaderPtr   = GRIResourcePtr<GRIVertexShader>;
    using GRIPixelShaderPtr    = GRIResourcePtr<GRIPixelShader>;
    using GRIPipelineStatePtr  = GRIResourcePtr<GRIPipelineState>;
    using GRIBufferPtr         = GRIResourcePtr<GRIBuffer>;

    enum
	{
		max_simultaneous_render_targets = 8,
	};

    enum class GRIResourceType
    {
        Viewport,
        Texture,
        Buffer,
        Shader,
        Pipeline,
        RenderTarget,
        Sampler,
        VertexDeclaration,
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

    enum class GRIShaderStage
    {
        Vertex,
        Pixel,
        Compute,
    };

    // Pixel formats shared across all backends.
    // Each backend maps these to its native format enum.
    enum class GRIPixelFormat
    {
        Unknown,
        RGBA8Unorm,
        BGRA8Unorm,
        Depth32Float,
    };

    enum class GRIPrimitiveTopology
    {
        TriangleList,
        TriangleStrip,
        LineList,
        PointList,
    };

    enum class GRIVertexElementFormat
    {
        Float1,
        Float2,
        Float3,
        Float4,
    };

    enum class GRIVertexElementSemantic
    {
        Position,
        Normal,
        TexCoord,
        Color,
    };

    enum class GRIBufferUsage : uint32_t
    {
        VertexBuffer  = 1 << 0,
        IndexBuffer   = 1 << 1,
        UniformBuffer = 1 << 2,
    };

    enum class GRIIndexFormat
    {
        Uint16,
        Uint32,
    };

    enum class GRILoadAction
    {
        Load,
        Clear,
        DontCare,
    };

    enum class GRIStoreAction
    {
        Store,
        DontCare,
    };

    struct GRIClearValue
    {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 1.0f;
    };
} // namespace Ignis
