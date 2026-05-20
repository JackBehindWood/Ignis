#pragma once

#include "AssetCompiler.h"
#include "Ignis/Rendering/ShaderCompiler.h"

namespace Ignis
{
    class AssetShaderCompiler : public AssetCompiler
    {
    public:
        explicit AssetShaderCompiler(ShaderTarget target);
        bool compile(const AssetMetadata& metadata) override;

    private:
        ShaderTarget   m_target;
        ShaderCompiler m_compiler;
    };

} // namespace Ignis
