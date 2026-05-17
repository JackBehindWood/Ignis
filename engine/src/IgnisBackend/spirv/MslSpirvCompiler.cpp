#include "igpch.h"
#include "MslSpirvCompiler.h"

#include <spirv_msl.hpp>

namespace Ignis
{

String MslSpirvCompiler::compile_native(const uint32_t* spirv, uint32_t word_count, spv::ExecutionModel exec_model)
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

} // namespace Ignis
