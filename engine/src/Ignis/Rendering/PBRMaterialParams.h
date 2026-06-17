#pragma once
#include "Ignis/Math/LinearColour.h"

namespace Ignis
{

struct alignas(16) PBRMaterialParams
{
    Math::LinearColour albedo_colour      = {1.0f, 1.0f, 1.0f, 1.0f}; // vec4 tint multiplied with albedo_tex
    Math::LinearColour emissive_colour    = {0.0f, 0.0f, 0.0f, 1.0f}; // vec4 tint multiplied with emissive_tex
    uint32_t           albedo_tex         = 0;
    uint32_t           normal_tex         = 0;
    uint32_t           roughness_tex      = 0;
    uint32_t           metallic_tex       = 0;
    uint32_t           ao_tex             = 0;
    uint32_t           emissive_tex       = 0;
    float              alpha_cutoff       = 0.0f;
    float              emissive_intensity = 1.0f;
};
static_assert(sizeof(PBRMaterialParams) == 64, "PBRMaterialParams layout mismatch with MaterialIndices");

} // namespace Ignis
