#pragma once

#include "Ignis/Asset/AssetLoader.h"

namespace Ignis
{
    class MaterialLoader : public AssetLoader
    {
    public:
        SharedPtr<Asset> load(const AssetMetadata& metadata) override;
    };

} // namespace Ignis
