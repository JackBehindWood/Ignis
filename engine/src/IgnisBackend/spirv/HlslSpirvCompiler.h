#pragma once

#include "Ignis/Rendering/SourceCompiler.h"

namespace Ignis
{
    // HLSL → SPIR-V compiler using DXC.
    // Implements SourceCompiler so it can be injected into ShaderCompiler.
    class HlslSpirvCompiler : public SourceCompiler
    {
    public:
        Vector<uint32_t> compile(const String& source, const char* entry_point, GRIShaderStage stage) override;
    };

} // namespace Ignis
