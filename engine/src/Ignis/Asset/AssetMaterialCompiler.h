#pragma once

#include "AssetCompiler.h"

namespace Ignis
{
    class AssetMaterialCompiler : public AssetCompiler
    {
    public:
        bool compile(const AssetMetadata& metadata) override;
    };

} // namespace Ignis
