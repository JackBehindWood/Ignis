#pragma once

#include "AssetHandler.h"

namespace Ignis
{

class MaterialHandler : public AssetHandler
{
public:
    bool             compile(const AssetMetadata& metadata) override;
    SharedPtr<Asset> load(const AssetMetadata& metadata) override;
};

} // namespace Ignis
