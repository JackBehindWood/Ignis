#include "igpch.h"
#include "SpirvLinker.h"
#include "SpirvUtils.h"

#include <spirv_cross.hpp>
#include <spirv.hpp>

namespace Ignis
{

InterfaceMap SpirvLinker::build_interface_map(const uint32_t* vs_spirv, uint32_t word_count)
{
    spirv_cross::Compiler        compiler(vs_spirv, word_count);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    InterfaceMap map;
    map.vs_output_locations.reserve(resources.stage_outputs.size());

    for (const auto& out : resources.stage_outputs)
    {
        const String semantic             = SpirvUtils::extract_semantic(compiler, out);
        map.vs_output_locations[semantic] = compiler.get_decoration(out.id, spv::DecorationLocation);
    }

    return map;
}

Vector<uint32_t> SpirvLinker::patch_ps_locations(const InterfaceMap& map, const uint32_t* ps_spirv, uint32_t word_count)
{
    // Phase A: reflect PS inputs and build variable_id → target_location patch list.
    spirv_cross::Compiler        ps_ref(ps_spirv, word_count);
    spirv_cross::ShaderResources ps_resources = ps_ref.get_shader_resources();

    UnorderedMap<uint32_t, uint32_t> patch_map; // variable_id → target Location
    for (const auto& in : ps_resources.stage_inputs)
    {
        const String semantic = SpirvUtils::extract_semantic(ps_ref, in);
        auto         it       = map.vs_output_locations.find(semantic);
        if (it == map.vs_output_locations.end())
        {
            continue;
        }
        const uint32_t current_location = ps_ref.get_decoration(in.id, spv::DecorationLocation);
        const uint32_t target_location  = it->second;
        if (current_location != target_location)
        {
            patch_map[in.id] = target_location;
        }
    }

    Vector<uint32_t> patched(ps_spirv, ps_spirv + word_count);

    if (patch_map.empty())
    {
        return patched;
    }

    // Phase B: walk the raw SPIR-V binary and apply patches.
    // SPIR-V header is 5 words; instructions begin at offset 5.
    uint32_t i = 5;
    while (i < word_count)
    {
        const uint32_t word0      = patched[i];
        const uint32_t opcode     = word0 & 0xFFFF;
        const uint32_t instr_size = word0 >> 16;

        if (instr_size == 0)
        {
            break;
        }

        if (opcode == 71 /* spv::Op::OpDecorate */ && instr_size >= 4)
        {
            const uint32_t decorated_id = patched[i + 1];
            const uint32_t decoration   = patched[i + 2];

            // Both conditions must hold — checking target ID alone would corrupt
            // Binding (33) or DescriptorSet (34) instructions on the same variable.
            if (decoration == 30 /* spv::DecorationLocation */ && patch_map.count(decorated_id))
            {
                patched[i + 3] = patch_map.at(decorated_id);
            }
        }

        i += instr_size;
    }

    return patched;
}

} // namespace Ignis
