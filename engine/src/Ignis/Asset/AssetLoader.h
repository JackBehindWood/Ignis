#pragma once

#include "Asset.h"
#include "Ignis/Foundation/SharedPtr.h"

#include "AssetBinaryStream.h"

namespace Ignis
{
    class AssetLoader
    {
    public:
        virtual ~AssetLoader() = default;
        virtual SharedPtr<Asset> load(const AssetMetadata& metadata) = 0;

    protected:
        AssetBinaryReader open_reader(const AssetMetadata& metadata, const AssetBlobHeader& header) const;
    };

} // namespace Ignis