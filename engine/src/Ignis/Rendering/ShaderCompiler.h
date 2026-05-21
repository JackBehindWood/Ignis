#pragma once

#include "ShaderTarget.h"
#include "ShaderReflection.h"
#include "GRI/GRIDefinitions.h"

namespace Ignis
{
    class SpirvCompiler;
    struct ShaderStageOutput
    {
        GRIShaderStage   stage;
        String           entry_point;
        Vector<uint8_t>  bytecode;
        ShaderReflection reflection;
    };

    struct ShaderCompilerOptions
    {
        struct StageEntry 
        { 
            GRIShaderStage stage; 
            String entry_point = ""; 
        };

        StageEntry stages[(size_t)GRIShaderStage::COUNT];
        uint8_t count = 0;
        Vector<Pair<String, String>> defines;
    };

    class ShaderCompiler
    {
    public:
        explicit ShaderCompiler(ShaderTarget target);
        ~ShaderCompiler();

        Vector<ShaderStageOutput> compile(const String& hlsl_source, const ShaderCompilerOptions& opts = {});

        ShaderTarget get_target() const { return m_target; }
    private:
        ShaderTarget             m_target;
        UniquePtr<SpirvCompiler> m_hlsl;
        UniquePtr<SpirvCompiler> m_backend;
    };

} // namespace Ignis
