#include "igpch.h"
#include "AssetMaterialCompiler.h"
#include "AssetBinaryStream.h"

namespace Ignis
{

// IGMT v2 — material recipe: shader source path, vertex layout name, param data blob.
static constexpr AssetBlobHeader k_material_header = {{'I', 'G', 'M', 'T'}, 2};

static void write_str(AssetBinaryWriter& w, const String& s)
{
    w.write_u32(static_cast<uint32_t>(s.size()));
    if (!s.empty())
        w.write_bytes(s.data(), s.size());
}

// .igmat text format:
//   line 1 — shader filename (e.g. "triangle.hlsl"), resolved to assets/shaders/
//   line 2 — optional vertex layout name (default: "standard_mesh")
bool AssetMaterialCompiler::compile(const AssetMetadata& metadata)
{
    String source_text;
    if (!read_source_text(metadata, source_text))
        return false;

    const auto trim = [](const String& s) -> String {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == String::npos) return {};
        size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    };

    String shader_filename;
    String vertex_layout = "standard_mesh";
    {
        size_t nl = source_text.find('\n');
        shader_filename = trim(nl != String::npos ? source_text.substr(0, nl) : source_text);
        if (nl != String::npos)
        {
            String line2 = trim(source_text.substr(nl + 1));
            if (!line2.empty())
                vertex_layout = line2;
        }
    }

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
    write_str(w, vertex_layout);
    w.write_u32(0); // param_data_size
    return w.good();
}

} // namespace Ignis
