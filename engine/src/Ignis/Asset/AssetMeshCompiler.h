#pragma once

#include "AssetCompiler.h"

namespace Ignis
{
    class AssetMeshCompiler : public AssetCompiler
    {
    public:
        bool compile(const AssetMetadata& metadata) override;
    };
}
