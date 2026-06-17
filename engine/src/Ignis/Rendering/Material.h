#pragma once

#include "Ignis/Rendering/GRI/GRIResource.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"
#include "Ignis/Rendering/Shaders/RenderShader.h"
#include "Ignis/Rendering/Shaders/MergedPipelineReflection.h"
#include "Ignis/Rendering/PBRMaterialParams.h"

namespace Ignis
{

class Material : public RefCounted
{
public:
    Material(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps, GRIPipelineStatePtr pso,
             const GRIVertexDeclaration* vd, MergedPipelineReflection pipeline_layout, GRIBufferPtr params = nullptr,
             GRIPipelineState* depth_pso = nullptr, GRIRasterDesc raster_desc = {}, GRIBlendDesc blend_desc = {},
             PBRMaterialParams pbr_params = {})
        : m_vs(std::move(vs)),
          m_ps(std::move(ps)),
          m_pso(std::move(pso)),
          m_vertex_decl(vd),
          m_pipeline_layout(std::move(pipeline_layout)),
          m_params_buffer(std::move(params)),
          m_depth_pso(depth_pso),
          m_raster_desc(raster_desc),
          m_blend_desc(blend_desc),
          m_pbr_params(pbr_params)
    {
    }

    GRIPipelineState* get_pipeline_state() const
    {
        return m_pso.get();
    }
    // Raw pointer; lifetime owned by MaterialFactory's PSO cache.
    GRIPipelineState* get_depth_pso() const
    {
        return m_depth_pso;
    }
    RenderShader* get_vertex_shader() const
    {
        return m_vs.get();
    }
    RenderShader* get_pixel_shader() const
    {
        return m_ps.get();
    }
    const GRIVertexDeclaration* get_vertex_declaration() const
    {
        return m_vertex_decl;
    }
    const MergedPipelineReflection& get_pipeline_layout() const
    {
        return m_pipeline_layout;
    }
    GRIBuffer* get_params_buffer() const
    {
        return m_params_buffer.get();
    }
    bool is_transparent() const
    {
        return m_blend_desc.enable;
    }
    const GRIRasterDesc& get_raster_desc() const
    {
        return m_raster_desc;
    }
    const GRIBlendDesc& get_blend_desc() const
    {
        return m_blend_desc;
    }
    const PBRMaterialParams& get_pbr_params() const
    {
        return m_pbr_params;
    }
    void update_pbr_params(const PBRMaterialParams& p);

private:
    SharedPtr<RenderShader>     m_vs;
    SharedPtr<RenderShader>     m_ps;
    GRIPipelineStatePtr         m_pso;
    const GRIVertexDeclaration* m_vertex_decl = nullptr; // borrowed from VertexDeclarationRegistry
    MergedPipelineReflection    m_pipeline_layout;
    GRIBufferPtr                m_params_buffer;
    GRIPipelineState*           m_depth_pso = nullptr; // borrowed from MaterialFactory PSO cache
    GRIRasterDesc               m_raster_desc;
    GRIBlendDesc                m_blend_desc;
    PBRMaterialParams           m_pbr_params;
};

} // namespace Ignis
