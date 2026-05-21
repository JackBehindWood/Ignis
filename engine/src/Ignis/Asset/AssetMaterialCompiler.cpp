#include "igpch.h"
#include "AssetMaterialCompiler.h"
#include "AssetBinaryStream.h"

namespace Ignis
{

// IGMT v1 — material recipe: absolute path to the HLSL shader source.
static constexpr AssetBlobHeader k_material_header = {{'I', 'G', 'M', 'T'}, 1};

static void write_str(AssetBinaryWriter& w, const String& s)
{
    w.write_u32(static_cast<uint32_t>(s.size()));
    if (!s.empty())
        w.write_bytes(s.data(), s.size());
}

// .igmat text format: single line containing the shader filename (e.g. "triangle.hlsl").
// Resolved relative to the sibling shaders/ directory.
bool AssetMaterialCompiler::compile(const AssetMetadata& metadata)
{
    String source_text;
    if (!read_source_text(metadata, source_text))
        return false;

    // Trim whitespace / newlines from the shader filename line.
    const auto trim = [](const String& s) -> String {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == String::npos) return {};
        size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    };
    const String shader_filename = trim(source_text);
    if (shader_filename.empty())
    {
        IG_CORE_ERROR("AssetMaterialCompiler: empty shader filename in '{}'", metadata.source_path.string());
        return false;
    }

    // materials/ → assets/ → assets/shaders/<filename>
    const Path shader_source = metadata.source_path.parent_path().parent_path() / "shaders" / shader_filename;
    if (!Filesystem::exists(shader_source))
    {
        IG_CORE_ERROR("AssetMaterialCompiler: shader not found: '{}'", shader_source.string());
        return false;
    }

    AssetBinaryWriter w = open_writer(metadata, k_material_header);
    if (!w.is_open())
        return false;

    write_str(w, shader_source.string());
    return w.good();
}

} // namespace Ignis
