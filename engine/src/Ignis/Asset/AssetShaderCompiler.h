#pragma once

#include "AssetCompiler.h"

namespace Ignis
{
    class AssetShaderCompiler : public AssetCompiler
    {
    public:
        bool compile(const AssetMetadata& metadata) override;
    };
}