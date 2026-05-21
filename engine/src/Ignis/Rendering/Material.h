#pragma once

#include "Ignis/Rendering/GRI/GRIResource.h"
#include "Ignis/Rendering/RenderShader.h"

namespace Ignis
{
    class Material : public RefCounted
    {
    public:
        Material(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps, GRIPipelineStatePtr pso)
            : m_vs(std::move(vs)), m_ps(std::move(ps)), m_pso(std::move(pso))
        {}

        GRIPipelineState* get_pipeline_state() const { return m_pso.get(); }
        RenderShader*     get_vertex_shader()  const { return m_vs.get(); }
        RenderShader*     get_pixel_shader()   const { return m_ps.get(); }

    private:
        SharedPtr<RenderShader> m_vs;
        SharedPtr<RenderShader> m_ps;
        GRIPipelineStatePtr     m_pso;
        // TODO: per-material parameter buffer (GRIBufferPtr m_parameter_buffer)
        // Blocked on material parameter system + vertex declaration registry
    };

} // namespace Ignis
