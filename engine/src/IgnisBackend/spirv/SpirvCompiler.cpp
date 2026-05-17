#include "igpch.h"
#include "SpirvCompiler.h"

#include <spirv_cross.hpp>

namespace Ignis
{

    namespace Utils
    {
        static spv::ExecutionModel to_execution_model(GRIShaderStage stage)
        {
            switch (stage)
            {
                case GRIShaderStage::Vertex:  return spv::ExecutionModelVertex;
                case GRIShaderStage::Pixel:   return spv::ExecutionModelFragment;
                case GRIShaderStage::Compute: return spv::ExecutionModelGLCompute;
                default:
                    IG_CORE_ASSERT(false, "SpirvCompiler: unsupported shader stage");
                    return spv::ExecutionModelVertex;
            }
        }

        static void read_bindings(const spirv_cross::Compiler& compiler, const spirv_cross::SmallVector<spirv_cross::Resource>& src, Vector<SpirvBindingInfo>& dst)
        {
            dst.reserve(src.size());
            for (const spirv_cross::Resource& r : src)
            {
                dst.push_back({ r.name, compiler.get_decoration(r.id, spv::DecorationDescriptorSet), compiler.get_decoration(r.id, spv::DecorationBinding),});
            }
        }
    }

    String SpirvCompiler::compile(const uint32_t* spirv, uint32_t word_count, GRIShaderStage stage)
    {
        return compile_native(spirv, word_count, Utils::to_execution_model(stage));
    }

    SpirvReflection SpirvCompiler::reflect(const uint32_t* spirv, uint32_t word_count)
    {
        spirv_cross::Compiler compiler(spirv, word_count);
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        SpirvReflection out;

        Utils::read_bindings(compiler, resources.uniform_buffers,   out.uniform_buffers);
        Utils::read_bindings(compiler, resources.storage_buffers,   out.storage_buffers);
        Utils::read_bindings(compiler, resources.separate_images,   out.separate_images);
        Utils::read_bindings(compiler, resources.separate_samplers, out.separate_samplers);

        out.stage_inputs.reserve(resources.stage_inputs.size());
        for (const auto& r : resources.stage_inputs)
        {
            out.stage_inputs.push_back({
                r.name,
                compiler.get_decoration(r.id, spv::DecorationLocation),
            });
        }

        out.push_constants.reserve(resources.push_constant_buffers.size());
        for (const auto& r : resources.push_constant_buffers)
        {
            const auto& type = compiler.get_type(r.base_type_id);
            out.push_constants.push_back({
                r.name,
                static_cast<uint32_t>(compiler.get_declared_struct_size(type)),
            });
        }

        return out;
    }

} // namespace Ignis
