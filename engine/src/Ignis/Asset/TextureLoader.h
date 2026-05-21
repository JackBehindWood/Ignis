#pragma once

#include "AssetLoader.h"

namespace Ignis
{
    class TextureLoader : public AssetLoader
    {
    public:
        SharedPtr<Asset> load(const AssetMetadata& metadata) override;
    };
}
