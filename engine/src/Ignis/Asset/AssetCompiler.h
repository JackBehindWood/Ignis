#pragma once

#include "Asset.h"

namespace Ignis
{

    class AssetCompiler
    {
    public:
        virtual ~AssetCompiler() = default;
        virtual bool compile(const AssetMetadata& metadata) = 0;
    };

} // namespace Ignis
