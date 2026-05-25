#pragma once

#include "AssetHandler.h"

namespace Ignis
{

class MeshHandler : public AssetHandler
{
public:
    AssetType        get_type() const override;
    bool             compile(const AssetMetadata& metadata) override;
    SharedPtr<Asset> load(const AssetMetadata& metadata) override;
    SharedPtr<Asset> create_default_fallback() const override;
    SharedPtr<Asset> load_from_bytes(const AssetMetadata& metadata, const Vector<uint8_t>& bytes) override;
};

} // namespace Ignis
