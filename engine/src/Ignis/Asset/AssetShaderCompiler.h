#pragma once

#include "AssetCompiler.h"

namespace Ignis
{
    // Writes a lightweight IGAS v1 recipe (.igasset) containing the source path and
    // entry points. Bytecode compilation is deferred to ShaderCache::get_or_compile().
    class AssetShaderCompiler : public AssetCompiler
    {
    public:
        bool compile(const AssetMetadata& metadata) override;
    };

} // namespace Ignis
