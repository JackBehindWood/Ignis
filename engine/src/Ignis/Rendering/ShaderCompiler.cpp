#include "igpch.h"
#include "ShaderCompiler.h"

#include "Ignis/Rendering/ShaderReflection.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"
#include "IgnisBackend/spirv/SpirvCompiler.h"

namespace Ignis
{

static ShaderReflection translate_reflection(const SpirvReflection& src)
{
    ShaderReflection r;
    for (const auto& b : src.uniform_buffers)
        r.uniform_buffers.push_back({b.name, b.set, b.binding});
    for (const auto& b : src.storage_buffers)
        r.storage_buffers.push_back({b.name, b.set, b.binding});
    for (const auto& b : src.separate_images)
        r.separate_images.push_back({b.name, b.set, b.binding});
    for (const auto& b : src.separate_samplers)
        r.separate_samplers.push_back({b.name, b.set, b.binding});
    for (const auto& i : src.stage_inputs)
        r.stage_inputs.push_back({i.name, i.location});
    for (const auto& p : src.push_constants)
        r.push_constants.push_back({p.name, p.size});
    return r;
}

// ---------------------------------------------------------------------------

ShaderCompiler::ShaderCompiler(ShaderTarget target)
    : m_target(target)
{}

Vector<ShaderStageOutput> ShaderCompiler::compile(const String& source)
{
    UniquePtr<SpirvCompiler> hlsl(SpirvCompiler::create(ShaderTarget::HLSL));
    const Vector<uint32_t> vs_spv = hlsl->compile_to_binary(source, "VSMain", GRIShaderStage::Vertex);
    const Vector<uint32_t> ps_spv = hlsl->compile_to_binary(source, "PSMain", GRIShaderStage::Pixel);
    if (vs_spv.empty() || ps_spv.empty())
        return {};

    const ShaderReflection vs_refl = translate_reflection(
        hlsl->reflect(vs_spv.data(), static_cast<uint32_t>(vs_spv.size())));
    const ShaderReflection ps_refl = translate_reflection(
        hlsl->reflect(ps_spv.data(), static_cast<uint32_t>(ps_spv.size())));

    UniquePtr<SpirvCompiler> compiler(SpirvCompiler::create(m_target));
    const Vector<uint8_t> vs_bin = compiler->spirv_to_backend_binary(
        vs_spv.data(), static_cast<uint32_t>(vs_spv.size()), GRIShaderStage::Vertex);
    const Vector<uint8_t> ps_bin = compiler->spirv_to_backend_binary(
        ps_spv.data(), static_cast<uint32_t>(ps_spv.size()), GRIShaderStage::Pixel);
    if (vs_bin.empty() || ps_bin.empty())
        return {};

    return {
        { GRIShaderStage::Vertex, "VSMain", vs_bin, vs_refl },
        { GRIShaderStage::Pixel,  "PSMain", ps_bin, ps_refl },
    };
}

} // namespace Ignis
