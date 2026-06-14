#pragma once

#include "Ignis/Foundation/String.h"
#include "Ignis/Foundation/Vector.h"
#include "Ignis/Foundation/UnorderedMap.h"

namespace Ignis
{

struct InterfaceMap
{
    UnorderedMap<String, uint32_t> vs_output_locations; // semantic → canonical Location
};

class SpirvLinker
{
public:
    // Reflect VS stage outputs and build a semantic → Location map.
    // VS SPIR-V is not modified; the returned map is canonical authority.
    static InterfaceMap build_interface_map(const uint32_t* vs_spirv, uint32_t word_count);

    // Produce a patched copy of ps_spirv where every PS input whose semantic appears in
    // map has its OpDecorate Location instruction updated to the canonical VS value.
    //
    // CRITICAL — dual validation per instruction:
    //   Both (target_id ∈ patch_map) AND (decoration == DecorationLocation == 30)
    //   must hold before overwriting. Checking target_id alone would corrupt
    //   Binding (33) or DescriptorSet (34) decorations on the same variable ID.
    static Vector<uint32_t> patch_ps_locations(const InterfaceMap& map, const uint32_t* ps_spirv, uint32_t word_count);
};

} // namespace Ignis
