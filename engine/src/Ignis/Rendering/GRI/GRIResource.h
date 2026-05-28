#pragma once

#include "GRIDefinitions.h"

namespace Ignis
{
class GRIResource : public RefCounted
{
public:
    explicit GRIResource(GRIResourceType type)
        : m_type(type)
    {
    }
    virtual ~GRIResource() = default;

    GRIResourceType get_type() const
    {
        return m_type;
    }

private:
    GRIResourceType m_type;
};

class GRIRenderTargetView
{
public:
    GRITexture2D* texture;
    uint32_t      mip_index;
    uint32_t      array_slice_index;
    GRIRenderTargetView()
        : texture(nullptr),
          mip_index(0),
          array_slice_index(-1)
    {
    }
    GRIRenderTargetView(GRITexture2D* texture)
        : texture(texture),
          mip_index(0),
          array_slice_index(-1)
    {
    }

    GRIRenderTargetView(GRIRenderTargetView&&)                 = default;
    GRIRenderTargetView(const GRIRenderTargetView&)            = default;
    GRIRenderTargetView& operator=(GRIRenderTargetView&&)      = default;
    GRIRenderTargetView& operator=(const GRIRenderTargetView&) = default;
};

class GRIDepthRenderTargetView
{
public:
    GRITexture2D* texture;
    GRIDepthRenderTargetView()
        : texture(nullptr)
    {
    }
    GRIDepthRenderTargetView(GRITexture2D* texture)
        : texture(texture)
    {
    }

    GRIDepthRenderTargetView(GRIDepthRenderTargetView&&)                 = default;
    GRIDepthRenderTargetView(const GRIDepthRenderTargetView&)            = default;
    GRIDepthRenderTargetView& operator=(GRIDepthRenderTargetView&&)      = default;
    GRIDepthRenderTargetView& operator=(const GRIDepthRenderTargetView&) = default;
};

class GRIRenderTargetsInfo
{
public:
    uint32_t                 num_targets;
    GRIRenderTargetView      colour_targets[max_simultaneous_render_targets];
    GRIDepthRenderTargetView depth_stencil_target;

    GRIRenderTargetsInfo()
        : num_targets(0),
          depth_stencil_target()
    {
    }
    GRIRenderTargetsInfo(uint32_t num_targets, const GRIRenderTargetView* render_target,
                         const GRIDepthRenderTargetView& depth_stencil_target)
        : num_targets(num_targets),
          depth_stencil_target(depth_stencil_target)
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
        GRITexture2D*  render_target     = nullptr;
        uint32_t       mip_index         = 0;
        uint32_t       array_slice_index = -1;
        GRILoadAction  load_action       = GRILoadAction::Clear;
        GRIStoreAction store_action      = GRIStoreAction::Store;
        GRIClearValue  clear_value;
    };

    struct DepthEntry
    {
        GRITexture2D*  depth_stencil_target = nullptr;
        GRILoadAction  load_action          = GRILoadAction::Clear;
        GRIStoreAction store_action         = GRIStoreAction::DontCare;
        float          clear_depth          = 1.0f;
    };

    ColourEntry colour_targets[max_simultaneous_render_targets];
    DepthEntry  depth_stencil_target;
    uint32_t    num_explicit_colour_targets = 0;

    GRIRenderPassInfo()                                    = default;
    GRIRenderPassInfo(const GRIRenderPassInfo&)            = default;
    GRIRenderPassInfo& operator=(const GRIRenderPassInfo&) = default;

    GRIRenderPassInfo(GRITexture2D* colour, uint32_t mip_index = 0, uint32_t array_slice_index = -1)
    {
        colour_targets[0].render_target     = colour;
        colour_targets[0].mip_index         = mip_index;
        colour_targets[0].array_slice_index = array_slice_index;
    }

    GRIRenderPassInfo(GRITexture2D* colour, uint32_t mip_index = 0, uint32_t array_slice_index = -1,
                      GRITexture2D* depth_stencil = nullptr)
    {
        colour_targets[0].render_target     = colour;
        colour_targets[0].mip_index         = mip_index;
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
    uint32_t       width             = 0;
    uint32_t       height            = 0;
    uint32_t       num_mip_levels    = 1;
    GRIPixelFormat format            = GRIPixelFormat::BGRA8Unorm;
    const void*    initial_data      = nullptr;
    size_t         initial_data_size = 0;
};

class GRITexture2D : public GRIResource
{
public:
    GRITexture2D()
        : GRIResource(GRIResourceType::Texture)
    {
    }

    virtual ~GRITexture2D()                = default;
    virtual uint32_t get_width() const     = 0;
    virtual uint32_t get_height() const    = 0;
    virtual uint32_t get_mip_count() const = 0;

    virtual void* get_native_handle() const = 0;
};

struct GRIShaderDesc
{
    GRIShaderStage stage         = GRIShaderStage::Vertex;
    const char*    entry_point   = nullptr;
    const uint8_t* bytecode_data = nullptr;
    size_t         bytecode_size = 0;
};

class GRIShader : public GRIResource
{
public:
    explicit GRIShader(GRIShaderStage stage)
        : GRIResource(GRIResourceType::Shader),
          m_stage(stage)
    {
    }
    virtual ~GRIShader() = default;
    GRIShaderStage get_stage() const
    {
        return m_stage;
    }

private:
    GRIShaderStage m_stage;
};

class GRIVertexShader : public GRIShader
{
public:
    GRIVertexShader()
        : GRIShader(GRIShaderStage::Vertex)
    {
    }
    virtual ~GRIVertexShader() = default;
};

class GRIPixelShader : public GRIShader
{
public:
    GRIPixelShader()
        : GRIShader(GRIShaderStage::Pixel)
    {
    }
    virtual ~GRIPixelShader() = default;
};

// ---------- Vertex layout ----------

// ---------- Buffers ----------

struct GRIBufferDesc
{
    uint32_t       size  = 0;
    GRIBufferUsage usage = GRIBufferUsage::VertexBuffer;
};

class GRIBuffer : public GRIResource
{
public:
    GRIBuffer()
        : GRIResource(GRIResourceType::Buffer)
    {
    }
    virtual ~GRIBuffer()                     = default;
    virtual uint32_t get_size() const        = 0;
    virtual void*    get_mapped_data() const = 0;
};

// ---------- Vertex layout ----------

struct GRIVertexElement
{
    GRIVertexElementSemantic semantic     = GRIVertexElementSemantic::Position;
    GRIVertexElementFormat   format       = GRIVertexElementFormat::Float3;
    uint32_t                 offset       = 0;
    uint32_t                 buffer_index = 0;
};

// One entry per bound vertex buffer — carries the stride that backends need
// when building their native vertex descriptor (MTLVertexDescriptor, VkVertexInputBindingDescription, etc.)
struct GRIVertexBufferBinding
{
    uint32_t buffer_index = 0;
    uint32_t stride       = 0;
};

// Pass nullptr in GRIPipelineStateDesc when the shader has no vertex inputs (e.g. hardcoded positions).
class GRIVertexDeclaration : public GRIResource
{
public:
    static constexpr uint32_t max_elements = 16;
    static constexpr uint32_t max_bindings = 8;

    GRIVertexElement elements[max_elements];
    uint32_t         num_elements = 0;

    GRIVertexBufferBinding bindings[max_bindings];
    uint32_t               num_bindings = 0;

    GRIVertexDeclaration()
        : GRIResource(GRIResourceType::VertexDeclaration)
    {
    }
    virtual ~GRIVertexDeclaration() = default;
};

struct GRIPipelineStateDesc
{
    GRIShader*            vertex_shader        = nullptr;
    GRIShader*            pixel_shader         = nullptr; // nullptr for depth-only passes
    GRIVertexDeclaration* vertex_declaration   = nullptr;
    GRIPixelFormat        render_target_format = GRIPixelFormat::RGBA8Unorm; // Unknown = no color attachment
    GRIPixelFormat        depth_stencil_format = GRIPixelFormat::Unknown;    // Unknown = no depth
    GRIPrimitiveTopology  primitive_topology   = GRIPrimitiveTopology::TriangleList;
    bool                  depth_write_enabled  = true;
    GRIBlendMode          blend_mode           = GRIBlendMode::None;
};

class GRIPipelineState : public GRIResource
{
public:
    GRIPipelineState()
        : GRIResource(GRIResourceType::Pipeline)
    {
    }
    virtual ~GRIPipelineState() = default;
};

struct GRIViewportDesc
{
    uint32_t width  = 800;
    uint32_t height = 600;

    const char* title = "GRIViewport";
};

class GRIViewport : public GRIResource
{
public:
    GRIViewport()
        : GRIResource(GRIResourceType::Viewport)
    {
    }

    virtual ~GRIViewport()              = default;
    virtual uint32_t get_width() const  = 0;
    virtual uint32_t get_height() const = 0;

    virtual void* get_native_handle() const = 0;
};
} // namespace Ignis
