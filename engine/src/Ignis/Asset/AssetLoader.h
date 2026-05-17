#pragma once

#include "Asset.h"
#include "Ignis/Foundation/SharedPtr.h"

namespace Ignis
{

    class AssetLoader
    {
    public:
        virtual ~AssetLoader() = default;
        virtual SharedPtr<Asset> load(const AssetMetadata& metadata) = 0;
    };

} // namespace Ignis
