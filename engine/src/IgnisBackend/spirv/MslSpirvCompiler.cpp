#include "igpch.h"
#include "MslSpirvCompiler.h"
#include "SpirvUtils.h"
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

// Apply uniform/storage buffer binding overrides, set MSL options, and compile.
// Called by both the standard and link-aware paths to avoid duplication.
static String apply_bindings_and_compile(spirv_cross::CompilerMSL& msl, const MslCompileOptions& options,
                                         spv::ExecutionModel exec_model)
{
    spirv_cross::ShaderResources resources = msl.get_shader_resources();

    for (const auto& resource : resources.uniform_buffers)
    {
        const uint32_t desc_set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        const uint32_t binding  = msl.get_decoration(resource.id, spv::DecorationBinding);

        spirv_cross::MSLResourceBinding msl_binding;
        msl_binding.stage    = exec_model;
        msl_binding.desc_set = desc_set;
        msl_binding.binding  = binding;
        // Use descriptor set as the Metal buffer index so each HLSL register space
        // maps to a distinct buffer slot. This prevents collision between uniforms
        // in different spaces that share binding=0 (e.g. g_frame space1 vs g_material space3).
        msl_binding.msl_buffer = desc_set;
        msl.add_msl_resource_binding(msl_binding);
    }

    // Storage buffers use the descriptor set as the MSL buffer slot so callers can select
    // an explicit Metal vertex-buffer index via HLSL register space<N>.
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

    for (const auto& resource : resources.storage_images)
    {
        const uint32_t desc_set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        const uint32_t binding  = msl.get_decoration(resource.id, spv::DecorationBinding);

        spirv_cross::MSLResourceBinding msl_binding;
        msl_binding.stage       = exec_model;
        msl_binding.desc_set    = desc_set;
        msl_binding.binding     = binding;
        msl_binding.msl_texture = binding;
        msl.add_msl_resource_binding(msl_binding);
    }

    for (const auto& resource : resources.separate_images)
    {
        const uint32_t desc_set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        const uint32_t binding  = msl.get_decoration(resource.id, spv::DecorationBinding);

        spirv_cross::MSLResourceBinding msl_binding;
        msl_binding.stage       = exec_model;
        msl_binding.desc_set    = desc_set;
        msl_binding.binding     = binding;
        msl_binding.msl_texture = binding;
        msl.add_msl_resource_binding(msl_binding);
    }

    for (const auto& resource : resources.separate_samplers)
    {
        const uint32_t desc_set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        const uint32_t binding  = msl.get_decoration(resource.id, spv::DecorationBinding);

        spirv_cross::MSLResourceBinding msl_binding;
        msl_binding.stage       = exec_model;
        msl_binding.desc_set    = desc_set;
        msl_binding.binding     = binding;
        msl_binding.msl_sampler = binding;
        msl.add_msl_resource_binding(msl_binding);
    }

    spirv_cross::CompilerMSL::Options opts;
    opts.platform = (options.platform == MslCompileOptions::Platform::iOS) ? spirv_cross::CompilerMSL::Options::iOS
                                                                           : spirv_cross::CompilerMSL::Options::macOS;
    opts.msl_version           = options.msl_version;
    opts.argument_buffers      = true;
    opts.argument_buffers_tier = spirv_cross::CompilerMSL::Options::ArgumentBuffersTier::Tier2;
    msl.set_msl_options(opts);

    // Space0 uses argument buffer indirection (unbounded bindless arrays on Apple Silicon).
    // Every other space uses direct discrete bindings to match CPU-side setVertexBuffer calls.
    for (uint32_t set = 1; set < 32; ++set)
    {
        msl.add_discrete_descriptor_set(set);
    }

    for (const auto& resource : resources.separate_images)
    {
        const uint32_t desc_set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        const auto&    type     = msl.get_type(resource.type_id);
        if (!type.array.empty() && type.array[0] == 0)
        {
            msl.set_argument_buffer_device_address_space(desc_set, true);
        }
    }

    for (const auto& resource : resources.separate_samplers)
    {
        const uint32_t desc_set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        const auto&    type     = msl.get_type(resource.type_id);
        if (!type.array.empty() && type.array[0] == 0)
        {
            msl.set_argument_buffer_device_address_space(desc_set, true);
        }
    }

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

} // namespace Ignis
