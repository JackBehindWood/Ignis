#include "igpch.h"
#include "MslSpirvCompiler.h"

#include <spirv_msl.hpp>

namespace Ignis
{

String MslSpirvCompiler::compile_from_target(const uint32_t* spirv, uint32_t word_count, spv::ExecutionModel exec_model)
{
    spirv_cross::CompilerMSL msl(spirv, word_count);

    // Force Metal buffer slots to match HLSL register bindings.
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

    spirv_cross::CompilerMSL::Options opts;
    opts.platform    = (m_options.platform == MslCompileOptions::Platform::iOS)
                           ? spirv_cross::CompilerMSL::Options::iOS
                           : spirv_cross::CompilerMSL::Options::macOS;
    opts.msl_version = m_options.msl_version;
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

Vector<uint8_t> MslSpirvCompiler::compile_to_backend(const uint32_t* spirv, uint32_t word_count, spv::ExecutionModel exec_model)
{
    const String msl = compile_from_target(spirv, word_count, exec_model);
    if (msl.empty())
        return {};

    const String uid        = to_string(reinterpret_cast<uintptr_t>(spirv));
    const Path   metal_path = Path("/tmp") / ("ig_" + uid + ".metal");
    const Path   air_path   = Path("/tmp") / ("ig_" + uid + ".air");
    const Path   lib_path   = Path("/tmp") / ("ig_" + uid + ".metallib");

    {
        // Refactored to use BinaryWriter instead of std::ofstream
        BinaryWriter writer(metal_path);
        if (!writer.is_open())
        {
            IG_CORE_ERROR("MslSpirvCompiler: failed to write temp .metal file");
            return {};
        }
        writer.write_bytes(msl.data(), msl.size());
    }

    const String metal_cmd = "xcrun -sdk macosx metal -c -o " + air_path.string() + " " + metal_path.string();
    if (std::system(metal_cmd.c_str()) != 0)
    {
        IG_CORE_ERROR("MslSpirvCompiler: xcrun metal failed");
        Filesystem::remove(metal_path);
        return {};
    }

    const String lib_cmd = "xcrun -sdk macosx metallib -o " + lib_path.string() + " " + air_path.string();
    if (std::system(lib_cmd.c_str()) != 0)
    {
        IG_CORE_ERROR("MslSpirvCompiler: xcrun metallib failed");
        Filesystem::remove(metal_path);
        Filesystem::remove(air_path);
        return {};
    }

    Vector<uint8_t> bytes;
    {
        // Refactored to use BinaryReader instead of std::ifstream
        BinaryReader reader(lib_path);
        if (reader.is_open())
        {
            size_t size = reader.get_size();
            bytes.resize(size);
            reader.read_bytes(bytes.data(), size);
        }
    }

    Filesystem::remove(metal_path);
    Filesystem::remove(air_path);
    Filesystem::remove(lib_path);

    return bytes;
}

} // namespace Ignis