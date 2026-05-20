#pragma once

#include "Ignis/Asset/AssetLoader.h"

namespace Ignis
{
    // Reads a compiled .igasset shader blob and constructs a Shader asset
    // by creating GRIVertexShader and GRIPixelShader objects through the active GRI.
    class ShaderLoader : public AssetLoader
    {
    public:
        SharedPtr<Asset> load(const AssetMetadata& metadata) override;
    };

} // namespace Ignis
