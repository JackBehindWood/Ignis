#pragma once

#include "AssetCompiler.h"

namespace Ignis
{
    class AssetTexture2DCompiler : public AssetCompiler
    {
    public:
        bool compile(const AssetMetadata& metadata) override;
    };
}
