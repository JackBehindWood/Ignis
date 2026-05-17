#pragma once

#include "Ignis/Asset/AssetCompiler.h"
#include "SourceCompiler.h"

namespace Ignis
{
    // Compiles a shader source file into a .igasset binary blob.
    // The source language is determined by the injected SourceCompiler (default: HLSL via DXC).
    //
    // .igasset binary layout (v2):
    //   [4] magic 'IGSH'  [1] version 2  [1] num_stages
    //   per stage: [1] stage_id  [4] word_count  [word_count * 4] SPIR-V words
    class ShaderCompiler : public AssetCompiler
    {
    public:
        explicit ShaderCompiler(UniquePtr<SourceCompiler> source_compiler);
        bool compile(const AssetMetadata& metadata) override;

    private:
        UniquePtr<SourceCompiler> m_source_compiler;
    };

} // namespace Ignis
