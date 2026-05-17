#include "igpch.h"
#include "ShaderLoader.h"
#include "Shader.h"

#include "Ignis/Asset/AssetBinaryStream.h"
#include "Ignis/Rendering/RenderSystem.h"

namespace Ignis
{

static constexpr AssetBlobHeader k_header = {{'I', 'G', 'S', 'H'}, 2};

SharedPtr<Asset> ShaderLoader::load(const AssetMetadata& metadata)
{
    AssetBinaryReader in = open_reader(metadata, k_header);
    if (!in.is_open())
        return nullptr;

    const uint8_t num_stages = in.read_u8();

    Vector<uint32_t> vs_spv, ps_spv;

    for (uint8_t i = 0; i < num_stages; ++i)
    {
        const GRIShaderStage stage      = static_cast<GRIShaderStage>(in.read_u8());
        const uint32_t       word_count = in.read_u32();

        Vector<uint32_t> spv(word_count);
        in.read_bytes(spv.data(), word_count * sizeof(uint32_t));

        if      (stage == GRIShaderStage::Vertex) vs_spv = std::move(spv);
        else if (stage == GRIShaderStage::Pixel)  ps_spv = std::move(spv);
    }

    if (vs_spv.empty() || ps_spv.empty())
    {
        IG_CORE_ERROR("ShaderLoader: missing stage data in {0}", metadata.compiled_path.string());
        return nullptr;
    }

    GRI* gri = RenderSystem::get_gri();
    if (!gri)
    {
        IG_CORE_ERROR("ShaderLoader: GRI not initialized");
        return nullptr;
    }

    GRIShaderDesc vs_desc;
    vs_desc.spirv       = vs_spv.data();
    vs_desc.spirv_size  = static_cast<uint32_t>(vs_spv.size());
    vs_desc.entry_point = "VSMain";
    vs_desc.stage       = GRIShaderStage::Vertex;

    GRIShaderDesc ps_desc;
    ps_desc.spirv       = ps_spv.data();
    ps_desc.spirv_size  = static_cast<uint32_t>(ps_spv.size());
    ps_desc.entry_point = "PSMain";
    ps_desc.stage       = GRIShaderStage::Pixel;

    GRIVertexShaderPtr vs = gri->create_vertex_shader(vs_desc);
    GRIPixelShaderPtr  ps = gri->create_pixel_shader(ps_desc);

    if (!vs || !ps)
    {
        IG_CORE_ERROR("ShaderLoader: GRI failed to create shaders from {0}", metadata.compiled_path.string());
        return nullptr;
    }

    return create_shared<Shader>(metadata.ID, std::move(vs), std::move(ps));
}

} // namespace Ignis
