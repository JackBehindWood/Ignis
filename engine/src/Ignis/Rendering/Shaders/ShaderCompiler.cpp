#include "igpch.h"
#include "ShaderCompiler.h"

#include "ShaderReflection.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"
#include "IgnisBackend/spirv/SpirvCompiler.h"
#include "IgnisBackend/spirv/HlslSpirvCompiler.h"
#include "IgnisBackend/spirv/SpirvLinker.h"

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
        r.uniform_buffers.push_back({b.name, b.set, b.binding, b.is_unbounded});
    }
    for (const auto& b : src.storage_buffers)
    {
        r.storage_buffers.push_back({b.name, b.set, b.binding, b.is_unbounded});
    }
    for (const auto& b : src.storage_textures)
    {
        r.storage_textures.push_back({b.name, b.set, b.binding, b.is_unbounded});
    }
    for (const auto& b : src.separate_images)
    {
        r.separate_images.push_back({b.name, b.set, b.binding, b.is_unbounded});
    }
    for (const auto& b : src.separate_samplers)
    {
        r.separate_samplers.push_back({b.name, b.set, b.binding, b.is_unbounded});
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
    r.threadgroup_size_x = src.threadgroup_size_x;
    r.threadgroup_size_y = src.threadgroup_size_y;
    r.threadgroup_size_z = src.threadgroup_size_z;
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

void ShaderCompiler::set_include_dirs(const Vector<Path>& dirs)
{
    static_cast<HlslSpirvCompiler*>(m_hlsl.get())->set_include_dirs(dirs);
}

// -------------------------------------------------------------------------

Vector<ShaderStageOutput> ShaderCompiler::compile(const String& source, const ShaderCompilerOptions& opts)
{
    if (!opts.include_dirs.empty())
    {
        static_cast<HlslSpirvCompiler*>(m_hlsl.get())->set_include_dirs(opts.include_dirs);
    }

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
    // Catches mismatches at compile time rather than deferring to pipeline-state creation.
    int vs_idx = -1;
    int ps_idx = -1;
    for (int i = 0; i < static_cast<int>(stage_ctx.size()); ++i)
    {
        if (stage_ctx[i].stage == GRIShaderStage::Vertex)
        {
            vs_idx = i;
        }
        if (stage_ctx[i].stage == GRIShaderStage::Pixel)
        {
            ps_idx = i;
        }
    }

    if (vs_idx >= 0 && ps_idx >= 0)
    {
        const StageContext& vs = stage_ctx[vs_idx];
        const StageContext& ps = stage_ctx[ps_idx];

        for (const auto& ps_in : ps.reflection.stage_inputs)
        {
            bool covered = false;
            for (const auto& vs_out : vs.reflection.stage_outputs)
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
                              ps.entry_point, ps_in.name, vs.entry_point);
                return {};
            }
        }

        // Phase 1c: Build canonical InterfaceMap from VS SPIR-V and patch PS input Location
        // decorations so all backends receive pre-linked, consistently-numbered SPIR-V.
        const StageContext& vs_ctx = stage_ctx[vs_idx];
        InterfaceMap        imap =
            SpirvLinker::build_interface_map(vs_ctx.spirv.data(), static_cast<uint32_t>(vs_ctx.spirv.size()));

        StageContext& ps_ctx = stage_ctx[ps_idx];
        ps_ctx.spirv =
            SpirvLinker::patch_ps_locations(imap, ps_ctx.spirv.data(), static_cast<uint32_t>(ps_ctx.spirv.size()));
    }

    // Phase 2: Pre-linked SPIR-V → backend binary.
    // All stages go through spirv_to_backend_binary; location patching already done in Phase 1c.
    Vector<ShaderStageOutput> result;
    result.reserve(stage_ctx.size());

    for (const auto& s : stage_ctx)
    {
        Vector<uint8_t> bin =
            m_backend->spirv_to_backend_binary(s.spirv.data(), static_cast<uint32_t>(s.spirv.size()), s.stage);
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
