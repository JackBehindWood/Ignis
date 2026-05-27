#include "igpch.h"
#include "SpirvCompiler.h"

#include <spirv_cross.hpp>

#include "IgnisBackend/spirv/HlslSpirvCompiler.h"

#ifdef IG_PLATFORM_MACOS
#include "IgnisBackend/spirv/MslSpirvCompiler.h"
#endif

namespace Ignis
{
namespace Utils
{
static void read_bindings(const spirv_cross::Compiler&                           compiler,
                          const spirv_cross::SmallVector<spirv_cross::Resource>& src, Vector<SpirvBindingInfo>& dst)
{
    dst.reserve(src.size());
    for (const spirv_cross::Resource& r : src)
    {
        dst.push_back({
            r.name,
            compiler.get_decoration(r.id, spv::DecorationDescriptorSet),
            compiler.get_decoration(r.id, spv::DecorationBinding),
        });
    }
}
} // namespace Utils

SpirvReflection SpirvCompiler::reflect(const uint32_t* spirv, uint32_t word_count)
{
    spirv_cross::Compiler        compiler(spirv, word_count);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    SpirvReflection out;

    Utils::read_bindings(compiler, resources.uniform_buffers, out.uniform_buffers);
    Utils::read_bindings(compiler, resources.storage_buffers, out.storage_buffers);
    Utils::read_bindings(compiler, resources.separate_images, out.separate_images);
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

UniquePtr<SpirvCompiler> SpirvCompiler::create(ShaderTarget target)
{
    switch (target)
    {
        case ShaderTarget::HLSL:
            return create_unique<HlslSpirvCompiler>();
#ifdef IG_PLATFORM_MACOS
        case ShaderTarget::Metal_MSL:
            return create_unique<MslSpirvCompiler>();
#else
        case ShaderTarget::Metal_MSL:
            IG_CORE_ERROR("Metal SPIR-V compilation is only supported on macOS");
            return nullptr;
#endif
        default:
            return nullptr; // TODO: make sure this is possible with UniquePtr and SharedPtr;
    }
}

} // namespace Ignis