#include "igpch.h"
#include "ShaderCompiler.h"

#include "ShaderReflection.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"
#include "IgnisBackend/spirv/SpirvCompiler.h"

namespace Ignis
{

constexpr const char* k_stage_default_entry[(size_t)GRIShaderStage::COUNT] = {
    "VSMain", // Vertex
    "PSMain", // Pixel
    "CSMain", // Compute
};

namespace Utils
{
static ShaderReflection translate_reflection(const SpirvReflection& src)
{
    ShaderReflection r;
    for (const auto& b : src.uniform_buffers)
    {
        r.uniform_buffers.push_back({b.name, b.set, b.binding});
    }
    for (const auto& b : src.storage_buffers)
    {
        r.storage_buffers.push_back({b.name, b.set, b.binding});
    }
    for (const auto& b : src.separate_images)
    {
        r.separate_images.push_back({b.name, b.set, b.binding});
    }
    for (const auto& b : src.separate_samplers)
    {
        r.separate_samplers.push_back({b.name, b.set, b.binding});
    }
    for (const auto& i : src.stage_inputs)
    {
        r.stage_inputs.push_back({i.name, i.location});
    }
    for (const auto& p : src.push_constants)
    {
        r.push_constants.push_back({p.name, p.size});
    }
    return r;
}
} // namespace Utils

// ---------------------------------------------------------------------------

ShaderCompiler::ShaderCompiler(ShaderTarget target)
    : m_target(target),
      m_hlsl(SpirvCompiler::create(ShaderTarget::HLSL)),
      m_backend(SpirvCompiler::create(target))
{
}

ShaderCompiler::~ShaderCompiler()
{
}

Vector<ShaderStageOutput> ShaderCompiler::compile(const String& source, const ShaderCompilerOptions& opts)
{
    Vector<ShaderStageOutput> result;
    result.reserve(opts.count);

    for (uint8_t i = 0; i < opts.count; ++i)
    {
        const ShaderCompilerOptions::StageEntry& req = opts.stages[i];
        const char*                              entry_point =
            req.entry_point.empty() ? k_stage_default_entry[(size_t)req.stage] : req.entry_point.c_str();
        const Vector<uint32_t> spv = m_hlsl->compile_to_binary(source, entry_point, req.stage, opts.defines);
        if (spv.empty())
        {
            return {};
        }

        const ShaderReflection refl =
            Utils::translate_reflection(m_hlsl->reflect(spv.data(), static_cast<uint32_t>(spv.size())));

        const Vector<uint8_t> bin =
            m_backend->spirv_to_backend_binary(spv.data(), static_cast<uint32_t>(spv.size()), req.stage);
        if (bin.empty())
        {
            return {};
        }

        result.push_back({req.stage, String(entry_point), bin, refl});
    }

    return result;
}

} // namespace Ignis
