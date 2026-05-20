#include "igpch.h"
#include "ShaderLoader.h"
#include "Ignis/Asset/AssetShader.h"
#include "ShaderTarget.h"
#include "ShaderReflection.h"

#include "Ignis/Asset/AssetBinaryStream.h"
#include "Ignis/Asset/AssetManager.h"
#include "Ignis/Rendering/RenderSystem.h"

namespace Ignis
{

static constexpr AssetBlobHeader k_header = {{'I', 'G', 'S', 'H'}, 7};

static String read_str(AssetBinaryReader& r)
{
    const uint32_t len = r.read_u32();
    String s(len, '\0');
    if (len > 0)
        r.read_bytes(s.data(), len);
    return s;
}

static ShaderReflection read_reflection(AssetBinaryReader& r)
{
    ShaderReflection refl;

    auto read_bindings = [&](Vector<ShaderResourceBinding>& v) {
        const uint32_t count = r.read_u32();
        v.resize(count);
        for (auto& b : v) { b.name = read_str(r); b.set = r.read_u32(); b.binding = r.read_u32(); }
    };
    read_bindings(refl.uniform_buffers);
    read_bindings(refl.storage_buffers);
    read_bindings(refl.separate_images);
    read_bindings(refl.separate_samplers);

    const uint32_t input_count = r.read_u32();
    refl.stage_inputs.resize(input_count);
    for (auto& i : refl.stage_inputs) { i.name = read_str(r); i.location = r.read_u32(); }

    const uint32_t pc_count = r.read_u32();
    refl.push_constants.resize(pc_count);
    for (auto& p : refl.push_constants) { p.name = read_str(r); p.size = r.read_u32(); }

    return refl;
}

struct StageData
{
    GRIShaderStage  stage;
    String          entry_point;
    Vector<uint8_t> bytecode;
    ShaderReflection reflection;
};

static bool read_stage(AssetBinaryReader& r, StageData& out)
{
    out.stage       = static_cast<GRIShaderStage>(r.read_u8());
    out.entry_point = read_str(r);
    const uint32_t sz = r.read_u32();
    out.bytecode.resize(sz);
    r.read_bytes(out.bytecode.data(), sz);
    out.reflection  = read_reflection(r);
    return r.good();
}

// ---------------------------------------------------------------------------

static SharedPtr<Asset> parse_and_create(const AssetMetadata& metadata)
{
    AssetBinaryReader r = AssetBinaryReader::open(metadata.compiled_path, k_header);
    if (!r.is_open())
        return nullptr;

    r.read_u8(); // target — GRI is already selected at runtime
    const uint8_t num_stages = r.read_u8();

    Vector<StageData> stages(num_stages);
    for (uint8_t i = 0; i < num_stages; ++i)
    {
        if (!read_stage(r, stages[i]))
            return nullptr;
    }

    GRI* gri = RenderSystem::get_gri();
    if (!gri)
        return nullptr;

    GRIVertexShaderPtr vs_shader;
    GRIPixelShaderPtr  ps_shader;
    ShaderReflection   vs_refl;
    ShaderReflection   ps_refl;

    for (auto& s : stages)
    {
        GRIShaderDesc desc;
        desc.stage         = s.stage;
        desc.entry_point   = s.entry_point.c_str();
        desc.bytecode_data = s.bytecode.data();
        desc.bytecode_size = s.bytecode.size();

        if (s.stage == GRIShaderStage::Vertex)
        {
            vs_shader = gri->create_vertex_shader(desc);
            vs_refl   = std::move(s.reflection);
        }
        else if (s.stage == GRIShaderStage::Pixel)
        {
            ps_shader = gri->create_pixel_shader(desc);
            ps_refl   = std::move(s.reflection);
        }
    }

    if (!vs_shader || !ps_shader)
        return nullptr;

    return create_shared<AssetShader>(metadata.ID,
        RenderShader(std::move(vs_shader), std::move(ps_shader),
                     std::move(vs_refl), std::move(ps_refl)));
}

SharedPtr<Asset> ShaderLoader::load(const AssetMetadata& metadata)
{
    SharedPtr<Asset> result = parse_and_create(metadata);
    if (result)
        return result;

    AssetCompiler* compiler = AssetManager::get_compiler(AssetType::Shader);
    if (!compiler || !compiler->compile(metadata))
    {
        IG_CORE_ERROR("ShaderLoader: cook failed for '{}'", metadata.source_path.string());
        return nullptr;
    }

    result = parse_and_create(metadata);
    if (!result)
        IG_CORE_ERROR("ShaderLoader: load failed after cook for '{}'", metadata.compiled_path.string());

    return result;
}

} // namespace Ignis
