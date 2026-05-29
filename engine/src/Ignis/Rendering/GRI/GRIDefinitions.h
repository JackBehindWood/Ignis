#pragma once

#include <Ignis/Foundation/Memory.h>

namespace Ignis
{
class GRI;
class GRICommandContext;
class GRICommandListBase;
class GRIResource;
class GRIViewport;
class GRITexture2D;
class GRIShader;
class GRIVertexShader;
class GRIPixelShader;
class GRIPipelineState;
class GRIVertexDeclaration;
class GRIBuffer;

template <typename T>
using GRIResourcePtr = SharedPtr<T>;

using GRIViewportPtr      = UniquePtr<GRIViewport>;
using GRITexture2DPtr     = GRIResourcePtr<GRITexture2D>;
using GRIShaderPtr        = GRIResourcePtr<GRIShader>;
using GRIVertexShaderPtr  = GRIResourcePtr<GRIVertexShader>;
using GRIPixelShaderPtr   = GRIResourcePtr<GRIPixelShader>;
using GRIPipelineStatePtr = GRIResourcePtr<GRIPipelineState>;
using GRIBufferPtr        = GRIResourcePtr<GRIBuffer>;

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
    None      = 0,
    OpenGL    = 1,
    Vulkan    = 2,
    DirectX12 = 3,
    Metal     = 4
};

enum class GRIShaderStage
{
    Vertex,
    Pixel,
    Compute,
    COUNT,
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
    Dynamic       = 1 << 3,
};

enum class GRIIndexFormat
{
    Uint16,
    Uint32,
};

enum class GRIBlendMode
{
    None,
    AlphaBlend,
};

enum class GRICompareFunc : uint8_t
{
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

enum class GRICullMode : uint8_t
{
    None,
    Front,
    Back
};
enum class GRIFillMode : uint8_t
{
    Solid,
    Wireframe
};

enum class GRIBlendFactor : uint8_t
{
    Zero,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DstAlpha,
    InvDstAlpha
};

enum class GRIBlendOp : uint8_t
{
    Add,
    Subtract,
    RevSubtract,
    Min,
    Max
};

struct GRIDepthStencilDesc
{
    bool           depth_test  = true;
    bool           depth_write = true;
    GRICompareFunc depth_func  = GRICompareFunc::LessEqual;
};

struct GRIRasterDesc
{
    GRICullMode cull_mode      = GRICullMode::Back;
    GRIFillMode fill_mode      = GRIFillMode::Solid;
    bool        front_face_ccw = true;
};

struct GRIBlendDesc
{
    bool           enable     = false;
    GRIBlendFactor src_factor = GRIBlendFactor::One;
    GRIBlendFactor dst_factor = GRIBlendFactor::Zero;
    GRIBlendOp     blend_op   = GRIBlendOp::Add;
    GRIBlendFactor src_alpha  = GRIBlendFactor::One;
    GRIBlendFactor dst_alpha  = GRIBlendFactor::Zero;
    GRIBlendOp     alpha_op   = GRIBlendOp::Add;
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
