#pragma once

#include "Ignis/Rendering/GRI/GRIResource.h"
#include "Ignis/Rendering/ShaderReflection.h"

namespace Ignis
{
    class RenderShader
    {
    public:
        RenderShader(GRIVertexShaderPtr vs, GRIPixelShaderPtr ps,
                     ShaderReflection vs_reflection, ShaderReflection ps_reflection)
            : m_vertex_shader(std::move(vs))
            , m_pixel_shader(std::move(ps))
            , m_vs_reflection(std::move(vs_reflection))
            , m_ps_reflection(std::move(ps_reflection))
        {}

        GRIVertexShader*       get_vertex_shader()  const { return m_vertex_shader.get(); }
        GRIPixelShader*        get_pixel_shader()   const { return m_pixel_shader.get(); }
        const ShaderReflection& get_vs_reflection() const { return m_vs_reflection; }
        const ShaderReflection& get_ps_reflection() const { return m_ps_reflection; }

    private:
        GRIVertexShaderPtr m_vertex_shader;
        GRIPixelShaderPtr  m_pixel_shader;
        ShaderReflection   m_vs_reflection;
        ShaderReflection   m_ps_reflection;
    };

} // namespace Ignis
