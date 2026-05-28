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
    virtual GRIViewportPtr  create_viewport(const GRIViewportDesc& desc)   = 0;

    virtual void resize_viewport(GRIViewport* viewport, uint32_t width, uint32_t height) = 0;

    virtual GRITexture2D* get_viewport_depth_texture(GRIViewport* viewport) = 0;

    virtual GRIVertexShaderPtr  create_vertex_shader(const GRIShaderDesc& desc)                  = 0;
    virtual GRIPixelShaderPtr   create_pixel_shader(const GRIShaderDesc& desc)                   = 0;
    virtual GRIPipelineStatePtr create_graphics_pipeline_state(const GRIPipelineStateDesc& desc) = 0;

    virtual GRIBufferPtr create_buffer(const GRIBufferDesc& desc, const void* initial_data = nullptr)           = 0;
    virtual void         update_buffer(GRIBuffer* buffer, const void* data, uint32_t size, uint32_t offset = 0) = 0;
};

} // namespace Ignis
