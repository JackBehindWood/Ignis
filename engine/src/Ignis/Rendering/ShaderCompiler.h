#pragma once

#include "ShaderTarget.h"
#include "ShaderReflection.h"
#include "GRI/GRIDefinitions.h"

namespace Ignis
{
    struct ShaderStageOutput
    {
        GRIShaderStage   stage;
        String           entry_point;
        Vector<uint8_t>  bytecode;
        ShaderReflection reflection;
    };

    class ShaderCompiler
    {
    public:
        explicit ShaderCompiler(ShaderTarget target);

        // HLSL source → per-stage compiled binaries + reflection. Returns empty on failure.
        Vector<ShaderStageOutput> compile(const String& hlsl_source);

        ShaderTarget get_target() const { return m_target; }

    private:
        ShaderTarget m_target;
    };

} // namespace Ignis
