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
class GRIComputeShader;
class GRIPipelineState;
class GRIComputePipelineState;
class GRIVertexDeclaration;
class GRIBuffer;
class GRISamplerState;

template <typename T>
using GRIResourcePtr = SharedPtr<T>;

using GRIViewportPtr             = UniquePtr<GRIViewport>;
using GRITexture2DPtr            = GRIResourcePtr<GRITexture2D>;
using GRIShaderPtr               = GRIResourcePtr<GRIShader>;
using GRIVertexShaderPtr         = GRIResourcePtr<GRIVertexShader>;
using GRIPixelShaderPtr          = GRIResourcePtr<GRIPixelShader>;
using GRIComputeShaderPtr        = GRIResourcePtr<GRIComputeShader>;
using GRIPipelineStatePtr        = GRIResourcePtr<GRIPipelineState>;
using GRIComputePipelineStatePtr = GRIResourcePtr<GRIComputePipelineState>;
using GRIBufferPtr               = GRIResourcePtr<GRIBuffer>;
using GRISamplerStatePtr         = GRIResourcePtr<GRISamplerState>;

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
    RGBA16Float,
    Depth32Float,
    RG16Float,
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
    VertexBuffer   = BIT(0),
    IndexBuffer    = BIT(1),
    UniformBuffer  = BIT(2),
    Dynamic        = BIT(3),
    StorageBuffer  = BIT(4),
    IndirectBuffer = BIT(5),
};

inline GRIBufferUsage operator|(GRIBufferUsage a, GRIBufferUsage b)
{
    return static_cast<GRIBufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline GRIBufferUsage operator&(GRIBufferUsage a, GRIBufferUsage b)
{
    return static_cast<GRIBufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline bool has_flag(GRIBufferUsage flags, GRIBufferUsage flag)
{
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

enum class GRIAccessFlags : uint32_t
{
    None                = 0,
    ComputeRead         = BIT(0),
    ComputeWrite        = BIT(1),
    GraphicsShaderRead  = BIT(2),
    RenderTarget        = BIT(3),
    DepthStencilWrite   = BIT(4),
    IndirectCommandRead = BIT(5),
    TransferSrc         = BIT(6),
    TransferDst         = BIT(7),
};

inline GRIAccessFlags operator|(GRIAccessFlags a, GRIAccessFlags b)
{
    return static_cast<GRIAccessFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline bool has_flag(GRIAccessFlags flags, GRIAccessFlags flag)
{
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

enum class GRIIndexFormat
{
    Uint16,
    Uint32,
};

enum class DefaultBindings : uint32_t
{
    FrameData      = 1,  // register(b0, space1)  — g_frame
    MaterialArgs   = 3,  // register(b0, space3)  — g_material
    InstanceData   = 27, // register(t0, space27) — g_instances (all-entity GPUInstanceData)
    VisibleIndices = 28, // register(t0, space28) — g_visible_indices (cull output)
    VertexBuffer   = 29, // k_default_vb_slot
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

struct GRISamplerDesc
{
    bool linear_filter = true;
    bool clamp_to_edge = true;
};

struct GRIComputePipelineStateDesc
{
    GRIShader* compute_shader     = nullptr;
    uint32_t   threadgroup_size_x = 1;
    uint32_t   threadgroup_size_y = 1;
    uint32_t   threadgroup_size_z = 1;
};

} // namespace Ignis
