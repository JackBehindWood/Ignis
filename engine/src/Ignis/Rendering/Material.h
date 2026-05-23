#pragma once

#include "Ignis/Rendering/GRI/GRIResource.h"
#include "Ignis/Rendering/RenderShader.h"

namespace Ignis
{
    class Material : public RefCounted
    {
    public:
        Material(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps, GRIPipelineStatePtr pso,
                 const GRIVertexDeclaration* vd, GRIBufferPtr params = nullptr, bool transparent = false)
            : m_vs(std::move(vs)), m_ps(std::move(ps)), m_pso(std::move(pso))
            , m_vertex_decl(vd), m_params_buffer(std::move(params)), m_transparent(transparent)
        {}

        GRIPipelineState*           get_pipeline_state()     const { return m_pso.get(); }
        RenderShader*               get_vertex_shader()      const { return m_vs.get(); }
        RenderShader*               get_pixel_shader()       const { return m_ps.get(); }
        const GRIVertexDeclaration* get_vertex_declaration() const { return m_vertex_decl; }
        GRIBuffer*                  get_params_buffer()      const { return m_params_buffer.get(); }
        bool                        is_transparent()         const { return m_transparent; }

    private:
        SharedPtr<RenderShader>     m_vs;
        SharedPtr<RenderShader>     m_ps;
        GRIPipelineStatePtr         m_pso;
        const GRIVertexDeclaration* m_vertex_decl   = nullptr; // borrowed from VertexDeclarationRegistry
        GRIBufferPtr                m_params_buffer;
        bool                        m_transparent   = false;
    };

} // namespace Ignis
