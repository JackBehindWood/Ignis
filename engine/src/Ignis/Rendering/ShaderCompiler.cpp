#include "igpch.h"
#include "ShaderCompiler.h"

#include "Ignis/Asset/AssetBinaryStream.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"

namespace Ignis
{

static constexpr AssetBlobHeader k_header = {{'I', 'G', 'S', 'H'}, 2}; // v2: stages store raw SPIR-V words

ShaderCompiler::ShaderCompiler(UniquePtr<SourceCompiler> source_compiler)
    : m_source_compiler(std::move(source_compiler))
{}

bool ShaderCompiler::compile(const AssetMetadata& metadata)
{
    String source;
    if (!read_source_text(metadata, source))
        return false;

    const Vector<uint32_t> vs_spv = m_source_compiler->compile(source, "VSMain", GRIShaderStage::Vertex);
    const Vector<uint32_t> ps_spv = m_source_compiler->compile(source, "PSMain", GRIShaderStage::Pixel);
    if (vs_spv.empty() || ps_spv.empty())
        return false;

    AssetBinaryWriter out = open_writer(metadata, k_header);
    if (!out.is_open())
        return !metadata.cache_compiled;

    out.write_u8(2); // num_stages

    out.write_u8(static_cast<uint8_t>(GRIShaderStage::Vertex));
    out.write_u32(static_cast<uint32_t>(vs_spv.size()));
    out.write_bytes(vs_spv.data(), vs_spv.size() * sizeof(uint32_t));

    out.write_u8(static_cast<uint8_t>(GRIShaderStage::Pixel));
    out.write_u32(static_cast<uint32_t>(ps_spv.size()));
    out.write_bytes(ps_spv.data(), ps_spv.size() * sizeof(uint32_t));

    IG_CORE_INFO("ShaderCompiler: compiled {0}", metadata.source_path.filename().string());
    return out.good();
}

} // namespace Ignis
