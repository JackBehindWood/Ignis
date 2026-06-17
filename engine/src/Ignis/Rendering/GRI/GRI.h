#pragma once

#include "GRIResource.h"

namespace Ignis
{
class GRI
{
public:
    static GRI* create(GRIRenderAPI api);

    virtual ~GRI() = default;

    virtual void init()     = 0;
    virtual void shutdown() = 0;

    virtual GRIRenderAPI       get_api() const = 0;
    virtual GRICommandContext* get_context()   = 0;

    virtual GRITexture2DPtr create_texture2d(const GRITexture2DDesc& desc) = 0;
    virtual GRITexture2DPtr create_cubemap(const GRITexture2DDesc& desc)   = 0;
    virtual GRIViewportPtr  create_viewport(const GRIViewportDesc& desc)   = 0;

    virtual void resize_viewport(GRIViewport* viewport, uint32_t width, uint32_t height) = 0;

    virtual GRITexture2D* get_viewport_depth_texture(GRIViewport* viewport) = 0;

    virtual GRIVertexShaderPtr         create_vertex_shader(const GRIShaderDesc& desc)                        = 0;
    virtual GRIPixelShaderPtr          create_pixel_shader(const GRIShaderDesc& desc)                         = 0;
    virtual GRIComputeShaderPtr        create_compute_shader(const GRIShaderDesc& desc)                       = 0;
    virtual GRIPipelineStatePtr        create_graphics_pipeline_state(const GRIPipelineStateDesc& desc)       = 0;
    virtual GRIComputePipelineStatePtr create_compute_pipeline_state(const GRIComputePipelineStateDesc& desc) = 0;

    virtual GRIBufferPtr create_buffer(const GRIBufferDesc& desc, const void* initial_data = nullptr)           = 0;
    virtual void         update_buffer(GRIBuffer* buffer, const void* data, uint32_t size, uint32_t offset = 0) = 0;

    virtual GRISamplerStatePtr create_sampler_state(const GRISamplerDesc& desc) = 0;

    // Synchronous GPU→CPU readback. Creates a dedicated blit command buffer, copies face/mip of
    // tex to a shared-mode staging buffer, commits, waits until completed, then memcpy to out.
    virtual void read_texture_sync(GRITexture2D* tex, uint32_t face, uint32_t mip, Vector<uint8_t>& out) = 0;

    // Evict a compiled shader function from the backend cache keyed by the fnv1a hash of its
    // bytecode. Called from ShaderCache::remove() to prevent stale MTL::Function* reuse on hot-reload.
    virtual void invalidate_compiled_shader(uint64_t bytecode_hash)
    {
    }

    // Initialize the bindless texture array argument encoder from a compiled PBR pixel shader.
    // Must be called once after the PBR shader is ready, before register_bindless_texture.
    // No-op on non-Metal backends.
    virtual void init_bindless_array(GRIShader* /*ps*/)
    {
    }

    // Register a texture+sampler pair in the global bindless array.
    // Returns the bindless slot index for use in PBRMaterialParams.
    // No-op on non-Metal backends — returns 0.
    virtual uint32_t register_bindless_texture(GRITexture2DPtr /*texture*/, GRISamplerStatePtr /*sampler*/)
    {
        return 0;
    }
};

} // namespace Ignis
