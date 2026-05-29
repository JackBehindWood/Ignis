#pragma once

#include "Ignis/Rendering/GRI/GRIResource.h"
#include "MetalAutoReleasePool.h"

#include <Metal/Metal.hpp>

struct GLFWwindow;

namespace CA
{
class MetalLayer;
class MetalDrawable;
} // namespace CA

namespace Ignis
{
class MetalGRI;
class MetalDevice;

class MetalTexture2D : public GRITexture2D
{
private:
    MetalDevice&  m_device;
    MTL::Texture* m_texture;
    uint32_t      m_width, m_height;
    uint32_t      m_mip_count;

public:
    MetalTexture2D(MetalDevice* device, const GRITexture2DDesc& desc);
    MetalTexture2D(MetalDevice* device, MTL::Texture* texture, bool retain = false);
    ~MetalTexture2D() override;
    virtual inline uint32_t get_width() const override
    {
        return m_width;
    };
    virtual inline uint32_t get_height() const override
    {
        return m_height;
    };
    virtual inline uint32_t get_mip_count() const override
    {
        return m_mip_count;
    };
    virtual inline void* get_native_handle() const override
    {
        return static_cast<void*>(m_texture);
    }

    inline MTL::Texture* get_texture()
    {
        return m_texture;
    }
};

// Non-owning GRITexture2D wrapper around a CA::MetalDrawable's texture.
// Valid only for the duration of the frame that acquired the drawable.
// Never calls retain/release — lifetime is managed by MetalViewport via the drawable.
class MetalTexture2DExtern final : public GRITexture2D
{
public:
    MetalTexture2DExtern() = default;
    explicit MetalTexture2DExtern(MTL::Texture* tex)
        : m_tex(tex)
    {
    }

    void reset(MTL::Texture* tex = nullptr)
    {
        m_tex = tex;
    }

    inline MTL::Texture* get_texture() const
    {
        return m_tex;
    }

    uint32_t get_width() const override
    {
        return m_tex ? static_cast<uint32_t>(m_tex->width()) : 0;
    }
    uint32_t get_height() const override
    {
        return m_tex ? static_cast<uint32_t>(m_tex->height()) : 0;
    }
    uint32_t get_mip_count() const override
    {
        return m_tex ? static_cast<uint32_t>(m_tex->mipmapLevelCount()) : 0;
    }
    void* get_native_handle() const override
    {
        return static_cast<void*>(m_tex);
    }

private:
    MTL::Texture* m_tex = nullptr;
};

class MetalViewport : public GRIViewport
{
private:
    MetalDevice&         m_device;
    MetalTexture2D*      m_depth_buffer;
    MetalTexture2DExtern m_drawable_view;
    uint32_t             m_width, m_height;

    GLFWwindow*        m_window;
    CA::MetalLayer*    m_metal_layer;
    CA::MetalDrawable* m_drawable;

    void setup_callbacks();
    void create_depth_buffer(uint32_t width, uint32_t height);
    void destroy_depth_buffer();

public:
    MetalViewport(MetalDevice* device, const GRIViewportDesc& desc);
    ~MetalViewport() override;

    inline MetalTexture2D* get_depth_buffer()
    {
        return m_depth_buffer;
    }
    inline MetalTexture2DExtern* get_drawable_view()
    {
        return &m_drawable_view;
    }

    void set_current_drawable(CA::MetalDrawable* drawable);

    void                    resize(uint32_t width, uint32_t height);
    virtual inline uint32_t get_width() const override
    {
        return m_width;
    };
    virtual inline uint32_t get_height() const override
    {
        return m_height;
    };

    inline CA::MetalDrawable* get_drawable() const
    {
        return m_drawable;
    }
    void release_drawable();

    virtual inline void* get_native_handle() const override
    {
        return static_cast<void*>(m_window);
    }

    inline MetalDevice& get_device()
    {
        return m_device;
    }
    inline GLFWwindow* get_window()
    {
        return m_window;
    }
    inline CA::MetalLayer* get_metal_layer()
    {
        return m_metal_layer;
    }
};

// ---------- Metal buffers ----------

class MetalBuffer : public GRIBuffer
{
private:
    MTL::Buffer* m_buffer;
    uint32_t     m_size;

public:
    MetalBuffer(MTL::Buffer* buffer, uint32_t size);
    ~MetalBuffer() override;
    uint32_t get_size() const override
    {
        return m_size;
    }
    void* get_mapped_data() const override
    {
        return m_buffer->contents();
    }
    MTL::Buffer* get_buffer() const
    {
        return m_buffer;
    }
};

// ---------- Metal shaders ----------

class MetalVertexShader : public GRIVertexShader
{
private:
    MTL::Function* m_function;

public:
    MetalVertexShader(MTL::Function* function);
    ~MetalVertexShader() override;
    inline MTL::Function* get_function() const
    {
        return m_function;
    }
};

class MetalPixelShader : public GRIPixelShader
{
private:
    MTL::Function* m_function;

public:
    MetalPixelShader(MTL::Function* function);
    ~MetalPixelShader() override;
    inline MTL::Function* get_function() const
    {
        return m_function;
    }
};

class MetalPipelineState : public GRIPipelineState
{
private:
    MTL::RenderPipelineState* m_pipeline_state;
    MTL::DepthStencilState*   m_depth_stencil_state;
    MTL::PrimitiveType        m_primitive_type;
    MTL::CullMode             m_cull_mode;
    MTL::TriangleFillMode     m_fill_mode;

public:
    MetalPipelineState(MTL::RenderPipelineState* pipeline_state, MTL::DepthStencilState* depth_stencil_state,
                       MTL::PrimitiveType primitive_type, MTL::CullMode cull_mode, MTL::TriangleFillMode fill_mode);
    ~MetalPipelineState() override;
    inline MTL::RenderPipelineState* get_pipeline_state() const
    {
        return m_pipeline_state;
    }
    inline MTL::DepthStencilState* get_depth_stencil_state() const
    {
        return m_depth_stencil_state;
    }
    inline MTL::PrimitiveType get_primitive_type() const
    {
        return m_primitive_type;
    }
    inline MTL::CullMode get_cull_mode() const
    {
        return m_cull_mode;
    }
    inline MTL::TriangleFillMode get_fill_mode() const
    {
        return m_fill_mode;
    }
};

template <class T>
struct MetalResourceTraits
{
};

template <>
struct MetalResourceTraits<GRITexture2D>
{
    typedef MetalTexture2D ConcreteType;
};

template <>
struct MetalResourceTraits<GRIViewport>
{
    typedef MetalViewport ConcreteType;
};

template <>
struct MetalResourceTraits<GRIBuffer>
{
    typedef MetalBuffer ConcreteType;
};

template <>
struct MetalResourceTraits<GRIVertexShader>
{
    typedef MetalVertexShader ConcreteType;
};

template <>
struct MetalResourceTraits<GRIPixelShader>
{
    typedef MetalPixelShader ConcreteType;
};

template <>
struct MetalResourceTraits<GRIPipelineState>
{
    typedef MetalPipelineState ConcreteType;
};

template <typename T>
static inline typename MetalResourceTraits<T>::ConcreteType* resource_cast(T* resource)
{
    return static_cast<typename MetalResourceTraits<T>::ConcreteType*>(resource);
}
} // namespace Ignis
