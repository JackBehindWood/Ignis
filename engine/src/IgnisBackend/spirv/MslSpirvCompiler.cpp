#include "igpch.h"
#include "MslSpirvCompiler.h"
#include "Ignis/Core/Shell.h"

#include <spirv_cross.hpp>
#include <spirv_msl.hpp>

namespace Ignis
{
namespace
{

static bool run_xcrun(const String& cmd)
{
    String    output;
    const int rc = Shell::exec(cmd, &output);
    if (rc != 0 && !output.empty())
    {
        IG_CORE_ERROR("MslSpirvCompiler: xcrun output:\n{}", output);
    }
#ifdef IG_DEBUG
    else if (!output.empty())
    {
        IG_CORE_TRACE("MslSpirvCompiler: xcrun output:\n{}", output);
    }
#endif
    return rc == 0;
}

// Extract the HLSL semantic from a stage I/O resource (mirrors SpirvCompiler::Utils::extract_semantic).
// Priority: DecorationHlslSemanticGOOGLE (5635) → strip DXC prefix → raw name.
static String extract_semantic(const spirv_cross::Compiler& compiler, const spirv_cross::Resource& r)
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

    // DXC emits dotted names on non-Windows ("in.var.SEMANTIC") and underscored names
    // on Windows ("in_var_SEMANTIC"). Handle both forms.
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

// Apply uniform/storage buffer binding overrides, set MSL options, and compile.
// Called by both the standard and link-aware paths to avoid duplication.
static String apply_bindings_and_compile(spirv_cross::CompilerMSL& msl, const MslCompileOptions& options,
                                         spv::ExecutionModel exec_model)
{
    spirv_cross::ShaderResources resources = msl.get_shader_resources();

    for (const auto& resource : resources.uniform_buffers)
    {
        const uint32_t binding = msl.get_decoration(resource.id, spv::DecorationBinding);

        spirv_cross::MSLResourceBinding msl_binding;
        msl_binding.stage      = exec_model;
        msl_binding.desc_set   = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        msl_binding.binding    = binding;
        msl_binding.msl_buffer = binding;
        msl.add_msl_resource_binding(msl_binding);
    }

    // Storage buffers use the descriptor set as the MSL buffer slot so callers can select
    // an explicit Metal vertex-buffer index via HLSL register space<N>.
    for (const auto& resource : resources.storage_buffers)
    {
        const uint32_t desc_set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        const uint32_t binding  = msl.get_decoration(resource.id, spv::DecorationBinding);

        spirv_cross::MSLResourceBinding msl_binding;
        msl_binding.stage      = exec_model;
        msl_binding.desc_set   = desc_set;
        msl_binding.binding    = binding;
        msl_binding.msl_buffer = desc_set;
        msl.add_msl_resource_binding(msl_binding);
    }

    spirv_cross::CompilerMSL::Options opts;
    opts.platform = (options.platform == MslCompileOptions::Platform::iOS) ? spirv_cross::CompilerMSL::Options::iOS
                                                                           : spirv_cross::CompilerMSL::Options::macOS;
    opts.msl_version = options.msl_version;
    msl.set_msl_options(opts);

    try
    {
        return msl.compile();
    }
    catch (const std::exception& e)
    {
        IG_CORE_ERROR("MslSpirvCompiler: SPIRV-Cross error: {}", e.what());
        return {};
    }
}

} // anonymous namespace

// -------------------------------------------------------------------------

String MslSpirvCompiler::compile_from_target(const uint32_t* spirv, uint32_t word_count, spv::ExecutionModel exec_model)
{
    spirv_cross::CompilerMSL msl(spirv, word_count);
    return apply_bindings_and_compile(msl, m_options, exec_model);
}

// -------------------------------------------------------------------------

Vector<uint8_t> MslSpirvCompiler::msl_to_metallib(const String& msl) const
{
    const String uid        = to_string(reinterpret_cast<uintptr_t>(msl.data()));
    const Path   metal_path = Path("/tmp") / ("ig_" + uid + ".metal");
    const Path   air_path   = Path("/tmp") / ("ig_" + uid + ".air");
    const Path   lib_path   = Path("/tmp") / ("ig_" + uid + ".metallib");

#ifdef IG_DEBUG
    IG_CORE_TRACE("MslSpirvCompiler: temp .metal source: {}", metal_path.string());
#endif

    {
        BinaryWriter writer(metal_path);
        if (!writer.is_open())
        {
            IG_CORE_ERROR("MslSpirvCompiler: failed to write temp .metal file");
            return {};
        }
        writer.write_bytes(msl.data(), msl.size());
    }

    const String metal_cmd = "xcrun -sdk macosx metal -c -o " + air_path.string() + " " + metal_path.string();
    if (!run_xcrun(metal_cmd))
    {
        IG_CORE_ERROR("MslSpirvCompiler: xcrun metal failed");
        Filesystem::remove(metal_path);
        return {};
    }

    const String lib_cmd = "xcrun -sdk macosx metallib -o " + lib_path.string() + " " + air_path.string();
    if (!run_xcrun(lib_cmd))
    {
        IG_CORE_ERROR("MslSpirvCompiler: xcrun metallib failed");
        Filesystem::remove(metal_path);
        Filesystem::remove(air_path);
        return {};
    }

    Vector<uint8_t> bytes;
    {
        BinaryReader reader(lib_path);
        if (reader.is_open())
        {
            const size_t size = reader.get_size();
            bytes.resize(size);
            reader.read_bytes(bytes.data(), size);
        }
    }

#ifndef IG_DEBUG
    Filesystem::remove(metal_path);
#endif
    Filesystem::remove(air_path);
    Filesystem::remove(lib_path);

    return bytes;
}

// -------------------------------------------------------------------------

Vector<uint8_t> MslSpirvCompiler::compile_to_backend(const uint32_t* spirv, uint32_t word_count,
                                                     spv::ExecutionModel exec_model)
{
    const String msl = compile_from_target(spirv, word_count, exec_model);
    if (msl.empty())
    {
        return {};
    }
    return msl_to_metallib(msl);
}

// -------------------------------------------------------------------------

Vector<uint8_t> MslSpirvCompiler::spirv_to_backend_binary_linked(const uint32_t* vs_spirv, uint32_t vs_word_count,
                                                                 const uint32_t* ps_spirv, uint32_t ps_word_count)
{
    // Step 1: Reflect VS outputs into a semantic → location map.
    spirv_cross::Compiler        vs_ref(vs_spirv, vs_word_count);
    spirv_cross::ShaderResources vs_resources = vs_ref.get_shader_resources();

    UnorderedMap<String, uint32_t> vs_out_locations;
    vs_out_locations.reserve(vs_resources.stage_outputs.size());
    for (const auto& out : vs_resources.stage_outputs)
    {
        vs_out_locations[extract_semantic(vs_ref, out)] = vs_ref.get_decoration(out.id, spv::DecorationLocation);
    }

    // Step 2: Construct a CompilerMSL for the PS.
    // CompilerMSL inherits Compiler::set_decoration(), so location decorations can be
    // patched directly on this instance before calling compile().
    spirv_cross::CompilerMSL     ps_msl(ps_spirv, ps_word_count);
    spirv_cross::ShaderResources ps_resources = ps_msl.get_shader_resources();

    for (const auto& in : ps_resources.stage_inputs)
    {
        const String semantic = extract_semantic(ps_msl, in);
        auto         it       = vs_out_locations.find(semantic);
        if (it != vs_out_locations.end())
        {
            ps_msl.set_decoration(in.id, spv::DecorationLocation, it->second);
        }
        else
        {
            IG_CORE_WARN("MslSpirvCompiler: PS input '{}' has no matching VS output — "
                         "verify the semantic name is correct.",
                         semantic);
        }
    }

    // Step 3: Apply resource bindings, MSL options, and compile.
    const String msl = apply_bindings_and_compile(ps_msl, m_options, spv::ExecutionModelFragment);
    if (msl.empty())
    {
        return {};
    }
    return msl_to_metallib(msl);
}

} // namespace Ignis
