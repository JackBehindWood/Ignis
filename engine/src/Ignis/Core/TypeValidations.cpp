#include "igpch.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"
#include "Ignis/Asset/AssetTypes.h"

namespace Ignis
{
static_assert(static_cast<uint8_t>(GRIShaderStage::Vertex) == 0,
              "GRIShaderStage binary encoding changed — update AssetShaderStage");
static_assert(static_cast<uint8_t>(GRIShaderStage::Pixel) == 1,
              "GRIShaderStage binary encoding changed — update AssetShaderStage");
static_assert(static_cast<uint8_t>(GRIShaderStage::Compute) == 2,
              "GRIShaderStage binary encoding changed — update AssetShaderStage");
static_assert(static_cast<uint8_t>(GRIPixelFormat::Unknown) == 0,
              "GRIPixelFormat binary encoding changed — update AssetPixelFormat");
static_assert(static_cast<uint8_t>(GRIPixelFormat::RGBA8Unorm) == 1,
              "GRIPixelFormat binary encoding changed — update AssetPixelFormat");
static_assert(static_cast<uint8_t>(GRIPixelFormat::BGRA8Unorm) == 2,
              "GRIPixelFormat binary encoding changed — update AssetPixelFormat");
static_assert(static_cast<uint8_t>(GRIPixelFormat::Depth32Float) == 3,
              "GRIPixelFormat binary encoding changed — update AssetPixelFormat");
} // namespace Ignis
