#pragma once

#include "AssetHandler.h"

namespace Ignis
{

class TextureHandler : public AssetHandler
{
public:
    bool             compile(const AssetMetadata& metadata) override;
    SharedPtr<Asset> load(const AssetMetadata& metadata) override;
    SharedPtr<Asset> load_from_bytes(const AssetMetadata& metadata,
                                      const Vector<uint8_t>& bytes) override;
};

} // namespace Ignis
