#pragma once

namespace Ignis
{
// Mirrors GRIShaderStage — values must match; static_asserts in GRIDefinitions enforce this.
enum class AssetShaderStage : uint8_t
{
    Vertex  = 0,
    Pixel   = 1,
    Compute = 2,
};

// Mirrors GRIPixelFormat — values must match; static_asserts in GRIDefinitions enforce this.
enum class AssetPixelFormat : uint8_t
{
    Unknown      = 0,
    RGBA8Unorm   = 1,
    BGRA8Unorm   = 2,
    RGBA16Float  = 3,
    Depth32Float = 4,
    RG16Float    = 5,
};
} // namespace Ignis
