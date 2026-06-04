#include "igpch.h"
#include "ShaderCompiler.h"

#include "ShaderReflection.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"
#include "IgnisBackend/spirv/SpirvCompiler.h"
#include "IgnisBackend/spirv/HlslSpirvCompiler.h"

namespace Ignis
{

constexpr const char* k_stage_default_entry[(size_t)GRIShaderStage::COUNT] = {
    "VSMain",
    "PSMain",
    "CSMain",
};

constexpr const char* k_stage_name[(size_t)GRIShaderStage::COUNT] = {
    "Vertex",
    "Pixel",
    "Compute",
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
    for (const auto& o : src.stage_outputs)
    {
        r.stage_outputs.push_back({o.name, o.location});
    }
    for (const auto& p : src.push_constants)
    {
        r.push_constants.push_back({p.name, p.size});
    }
    return r;
}
} // namespace Utils

// -------------------------------------------------------------------------

ShaderCompiler::ShaderCompiler(ShaderTarget target)
    : m_target(target),
      m_hlsl(SpirvCompiler::create(ShaderTarget::HLSL)),
      m_backend(SpirvCompiler::create(target))
{
}

ShaderCompiler::~ShaderCompiler() = default;

void ShaderCompiler::register_virtual_include(const String& virtual_path, const String& source)
{
    static_cast<HlslSpirvCompiler*>(m_hlsl.get())->register_virtual_include(virtual_path, source);
}

void ShaderCompiler::set_source_directory(const Path& dir)
{
    static_cast<HlslSpirvCompiler*>(m_hlsl.get())->set_source_directory(dir);
}

// -------------------------------------------------------------------------

Vector<ShaderStageOutput> ShaderCompiler::compile(const String& source, const ShaderCompilerOptions& opts)
{
    // Phase 1: HLSL → SPIR-V + reflection for every requested stage.
    struct StageContext
    {
        GRIShaderStage   stage;
        const char*      entry_point;
        Vector<uint32_t> spirv;
        SpirvReflection  reflection;
    };

    Vector<StageContext> stage_ctx;
    stage_ctx.reserve(opts.count);

    for (uint8_t i = 0; i < opts.count; ++i)
    {
        const auto& req = opts.stages[i];
        const char* ep  = req.entry_point.empty() ? k_stage_default_entry[(size_t)req.stage] : req.entry_point.c_str();

        Vector<uint32_t> spv = m_hlsl->compile_to_binary(source, ep, req.stage, opts.defines);
        if (spv.empty())
        {
            IG_CORE_ERROR("ShaderCompiler: HLSL→SPIR-V failed (stage={}, entry='{}')", k_stage_name[(size_t)req.stage],
                          ep);
            return {};
        }

        SpirvReflection refl = m_hlsl->reflect(spv.data(), static_cast<uint32_t>(spv.size()));
        stage_ctx.push_back({req.stage, ep, std::move(spv), std::move(refl)});
    }

    // Phase 1b: VS↔PS interface validation using semantic names.
    // This catches mismatches at compile time with full entry-point context rather than
    // deferring them to pipeline-state creation or GPU validation.
    const StageContext* vs_ctx = nullptr;
    const StageContext* ps_ctx = nullptr;
    for (const auto& s : stage_ctx)
    {
        if (s.stage == GRIShaderStage::Vertex)
        {
            vs_ctx = &s;
        }
        if (s.stage == GRIShaderStage::Pixel)
        {
            ps_ctx = &s;
        }
    }

    if (vs_ctx && ps_ctx)
    {
        for (const auto& ps_in : ps_ctx->reflection.stage_inputs)
        {
            bool covered = false;
            for (const auto& vs_out : vs_ctx->reflection.stage_outputs)
            {
                if (vs_out.name == ps_in.name)
                {
                    covered = true;
                    break;
                }
            }
            if (!covered)
            {
                IG_CORE_ERROR("ShaderCompiler: stage interface mismatch — "
                              "PS entry '{}' expects input '{}' but VS entry '{}' does not export it.",
                              ps_ctx->entry_point, ps_in.name, vs_ctx->entry_point);
                return {};
            }
        }
    }

    // Phase 2: SPIR-V → backend binary.
    // PS stages are compiled with link-aware path when a VS is present so that
    // DCE-induced Location renumbering is corrected before MSL generation.
    Vector<ShaderStageOutput> result;
    result.reserve(stage_ctx.size());

    for (const auto& s : stage_ctx)
    {
        Vector<uint8_t> bin;

        if (s.stage == GRIShaderStage::Pixel && vs_ctx)
        {
            bin = m_backend->spirv_to_backend_binary_linked(vs_ctx->spirv.data(),
                                                            static_cast<uint32_t>(vs_ctx->spirv.size()), s.spirv.data(),
                                                            static_cast<uint32_t>(s.spirv.size()));
        }
        else
        {
            bin = m_backend->spirv_to_backend_binary(s.spirv.data(), static_cast<uint32_t>(s.spirv.size()), s.stage);
        }

        if (bin.empty())
        {
            IG_CORE_ERROR("ShaderCompiler: SPIR-V→backend failed (stage={}, entry='{}')", k_stage_name[(size_t)s.stage],
                          s.entry_point);
            return {};
        }

        result.push_back({s.stage, String(s.entry_point), std::move(bin), Utils::translate_reflection(s.reflection)});
    }

    return result;
}

} // namespace Ignis
