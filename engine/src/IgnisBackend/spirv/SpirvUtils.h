#pragma once

#include <spirv_cross.hpp>

#include "Ignis/Foundation/String.h"

namespace Ignis::SpirvUtils
{

// Extract the HLSL semantic string from a stage I/O resource.
// Priority:
//   1. DecorationHlslSemanticGOOGLE (5635) — present on all DXC-compiled SPIR-V.
//   2. Strip "in_var_" / "out_var_" / "in.var." / "out.var." prefix — DXC fallback.
//   3. Raw variable name — hand-assembled SPIR-V with no DXC conventions.
static inline String extract_semantic(const spirv_cross::Compiler& compiler, const spirv_cross::Resource& r)
{
    constexpr auto k_hlsl_semantic = static_cast<spv::Decoration>(5635);
    if (compiler.has_decoration(r.id, k_hlsl_semantic))
    {
        String s = compiler.get_decoration_string(r.id, k_hlsl_semantic);
        if (!s.empty())
        {
            return s;
        }
    }

    const String& name = r.name;
    for (const char* prefix : {"in.var.", "out.var.", "in_var_", "out_var_"})
    {
        const size_t plen = std::strlen(prefix);
        if (name.size() > plen && name.compare(0, plen, prefix) == 0)
        {
            return name.substr(plen);
        }
    }

    return name;
}

} // namespace Ignis::SpirvUtils
