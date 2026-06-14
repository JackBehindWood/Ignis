#include "Ignis/Rendering/GRI/GRI.h"
#include "MetalContext.h"

#include "MetalResource.h"

namespace Ignis
{
class MetalGRI : public GRI
{
private:
    MetalDevice*        m_device;
    MetalCommandContext m_context;

public:
    MetalGRI();
    ~MetalGRI() = default;

    void init() override;
    void shutdown() override;

    GRITexture2DPtr create_texture2d(const GRITexture2DDesc& desc) override;
    GRITexture2DPtr create_cubemap(const GRITexture2DDesc& desc) override;
    GRIViewportPtr  create_viewport(const GRIViewportDesc& desc) override;

    void          resize_viewport(GRIViewport* viewport, uint32_t width, uint32_t height) override;
    GRITexture2D* get_viewport_depth_texture(GRIViewport* viewport) override;

    GRIVertexShaderPtr         create_vertex_shader(const GRIShaderDesc& desc) override;
    GRIPixelShaderPtr          create_pixel_shader(const GRIShaderDesc& desc) override;
    GRIComputeShaderPtr        create_compute_shader(const GRIShaderDesc& desc) override;
    GRIPipelineStatePtr        create_graphics_pipeline_state(const GRIPipelineStateDesc& desc) override;
    GRIComputePipelineStatePtr create_compute_pipeline_state(const GRIComputePipelineStateDesc& desc) override;

    GRIBufferPtr create_buffer(const GRIBufferDesc& desc, const void* initial_data = nullptr) override;
    void         update_buffer(GRIBuffer* buffer, const void* data, uint32_t size, uint32_t offset = 0) override;

    GRISamplerStatePtr create_sampler_state(const GRISamplerDesc& desc) override;
    void               read_texture_sync(GRITexture2D* tex, uint32_t face, uint32_t mip, Vector<uint8_t>& out) override;

    void invalidate_compiled_shader(uint64_t bytecode_hash) override;

    inline GRIRenderAPI get_api() const override
    {
        return GRIRenderAPI::Metal;
    }

    inline MetalDevice* get_device() const
    {
        return m_device;
    }

    inline GRICommandContext* get_context() override
    {
        return &m_context;
    }
};
} // namespace Ignis
