#include "igpch.h"
#include "MaterialHandler.h"

#include "Ignis/Asset/AssetMaterial.h"
#include "Ignis/Asset/AssetBinaryStream.h"

namespace Ignis
{

// IGMT v2 — material recipe: shader source path, vertex layout name, param data blob.
static constexpr AssetBlobHeader k_material_header = {{'I', 'G', 'M', 'T'}, 2};

// ---------------------------------------------------------------------------

bool MaterialHandler::compile(const AssetMetadata& metadata)
{
    String source_text;
    if (!read_source_text(metadata, source_text))
    {
        IG_CORE_ERROR("MaterialHandler: failed to read source for '{}'", metadata.source_path.string());
        return false;
    }

    const auto trim = [](const String& s) -> String
    {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == String::npos)
        {
            return {};
        }
        size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    };

    String shader_filename;
    String vertex_layout = "standard_mesh";
    {
        const size_t nl = source_text.find('\n');
        shader_filename = trim(nl != String::npos ? source_text.substr(0, nl) : source_text);
        if (nl != String::npos)
        {
            String line2 = trim(source_text.substr(nl + 1));
            if (!line2.empty())
            {
                vertex_layout = line2;
            }
        }
    }

    if (shader_filename.empty())
    {
        IG_CORE_ERROR("MaterialHandler: empty shader filename in '{}'", metadata.source_path.string());
        return false;
    }

    const Path shader_source = metadata.source_path.parent_path().parent_path() / "shaders" / shader_filename;
    if (!Filesystem::exists(shader_source))
    {
        IG_CORE_ERROR("MaterialHandler: shader not found: '{}'", shader_source.string());
        return false;
    }

    AssetBinaryWriter w = open_writer(metadata, k_material_header);
    if (!w.is_open())
    {
        IG_CORE_ERROR("MaterialHandler: could not open output '{}'", metadata.compiled_path.string());
        return false;
    }

    write_string(w, shader_source.string());
    write_string(w, vertex_layout);
    w.write_u32(0); // param_data_size
    return w.finalize();
}

template <typename R>
static SharedPtr<Asset> parse_material_payload(const AssetMetadata& metadata, R& r)
{
    const Path     shader_source(MaterialHandler::read_string(r));
    const String   vertex_layout_name = MaterialHandler::read_string(r);
    const uint32_t param_data_size    = r.read_u32();

    Vector<uint8_t> param_data;
    if (param_data_size > 0)
    {
        param_data.resize(param_data_size);
        r.read_bytes(param_data.data(), param_data_size);
    }

    if (!r.good())
    {
        IG_CORE_ERROR("MaterialHandler: corrupted recipe for '{}'", metadata.compiled_path.string());
        return nullptr;
    }

    return create_shared<AssetMaterial>(metadata.ID, shader_source, vertex_layout_name, std::move(param_data));
}

SharedPtr<Asset> MaterialHandler::load(const AssetMetadata& metadata)
{
    AssetBinaryReader r = open_reader(metadata, k_material_header);
    if (!r.is_open())
    {
        if (!compile(metadata))
        {
            IG_CORE_ERROR("MaterialHandler: recipe cook failed for '{}'", metadata.source_path.string());
            return nullptr;
        }
        r = open_reader(metadata, k_material_header);
        if (!r.is_open())
        {
            IG_CORE_ERROR("MaterialHandler: failed to open compiled asset for '{}'", metadata.compiled_path.string());
            return nullptr;
        }
    }

    SharedPtr<Asset> asset = parse_material_payload(metadata, r);
    if (!asset)
    {
        IG_CORE_ERROR("MaterialHandler: corrupted binary for '{}'", metadata.compiled_path.string());
    }
    return asset;
}

SharedPtr<Asset> MaterialHandler::load_from_bytes(const AssetMetadata& metadata, const Vector<uint8_t>& bytes)
{
    MemBinaryReader  r(bytes.data(), bytes.size());
    SharedPtr<Asset> asset = parse_material_payload(metadata, r);
    if (!asset)
    {
        IG_CORE_ERROR("MaterialHandler: corrupted payload for '{}'", metadata.compiled_path.string());
    }
    return asset;
}

AssetType MaterialHandler::get_type() const
{
    return AssetType::Material;
}
SharedPtr<Asset> MaterialHandler::create_default_fallback() const
{
    return nullptr;
}

} // namespace Ignis
