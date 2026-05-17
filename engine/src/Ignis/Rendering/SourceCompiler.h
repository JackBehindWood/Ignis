#pragma once

#include "Ignis/Rendering/GRI/GRIDefinitions.h"

namespace Ignis
{
    // Compiles backend-agnostic shader source into SPIR-V words.
    // Implement per source language (HLSL, GLSL, etc.).
    class SourceCompiler
    {
    public:
        virtual ~SourceCompiler() = default;
        virtual Vector<uint32_t> compile(const String& source, const char* entry_point, GRIShaderStage stage) = 0;
    };

} // namespace Ignis
