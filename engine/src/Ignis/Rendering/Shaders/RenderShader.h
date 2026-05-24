#pragma once

#include "ShaderReflection.h"
#include "Ignis/Rendering/GRI/GRIResource.h"

namespace Ignis
{
    class RenderShader : public RefCounted
    {
    public:
        RenderShader(GRIShaderPtr shader, GRIShaderStage stage, ShaderReflection reflection)
            : m_shader(std::move(shader))
            , m_stage(stage)
            , m_reflection(std::move(reflection))
        {}

        GRIShader*              get_shader()     const { return m_shader.get(); }
        GRIShaderStage          get_stage()      const { return m_stage; }
        const ShaderReflection& get_reflection() const { return m_reflection; }

    private:
        GRIShaderPtr     m_shader;
        GRIShaderStage   m_stage;
        ShaderReflection m_reflection;
    };

} // namespace Ignis
