#include "igpch.h"
#include "AssetHandler.h"
#include <chrono>

#include "ShaderHandler.h"
#include "MeshHandler.h"
#include "MaterialHandler.h"
#include "TextureHandler.h"

namespace Ignis
{
    AssetHandler *AssetHandler::get(AssetType type)
    {
        static ShaderHandler   s_shader_handler;
        static MeshHandler     s_mesh_handler;
        static TextureHandler  s_texture_handler;
        static MaterialHandler s_material_handler;
        
        switch (type)
        {
            case AssetType::Shader:    return &s_shader_handler;
            case AssetType::Mesh:      return &s_mesh_handler;
            case AssetType::Texture2D: return &s_texture_handler;
            case AssetType::Material:  return &s_material_handler;
            default:                   return nullptr;
        }
    }

bool AssetHandler::read_source_text(const AssetMetadata& metadata, String& out_text) const
{
    if (!Filesystem::exists(metadata.source_path))
    {
        IG_CORE_ERROR("AssetHandler: source not found: {0}", metadata.source_path.string());
        return false;
    }
    BinaryReader src(metadata.source_path);
    if (!src.is_open())
    {
        IG_CORE_ERROR("AssetHandler: failed to open source: {0}", metadata.source_path.string());
        return false;
    }
    out_text = src.read_all_text();
    return true;
}

bool AssetHandler::read_source_bytes(const AssetMetadata& metadata, Vector<uint8_t>& out) const
{
    if (!Filesystem::exists(metadata.source_path))
    {
        IG_CORE_ERROR("AssetHandler: source not found: {0}", metadata.source_path.string());
        return false;
    }
    BinaryReader src(metadata.source_path);
    if (!src.is_open())
    {
        IG_CORE_ERROR("AssetHandler: failed to open source: {0}", metadata.source_path.string());
        return false;
    }
    const size_t size = src.get_size();
    out.resize(size);
    src.read_bytes(out.data(), size);
    return true;
}

bool AssetHandler::ensure_output_dir(const AssetMetadata& metadata) const
{
    const Path dir = metadata.compiled_path.parent_path();
    if (!dir.empty() && !Filesystem::exists(dir))
    {
        std::error_code ec;
        Filesystem::create_directories(dir, ec);
        if (ec)
        {
            IG_CORE_ERROR("AssetHandler: failed to create output dir: {0}", dir.string());
            return false;
        }
    }
    return true;
}

AssetBinaryWriter AssetHandler::open_writer(const AssetMetadata& metadata,
                                             const AssetBlobHeader& header,
                                             const Vector<AssetID>& deps) const
{
    ensure_output_dir(metadata);
    return AssetBinaryWriter::open(metadata, header, metadata.Type, deps);
}

AssetBinaryReader AssetHandler::open_reader(const AssetMetadata& metadata,
                                             const AssetBlobHeader& header) const
{
    AssetBinaryReader reader = AssetBinaryReader::open(metadata, header);
    if (!reader.is_open())
        IG_CORE_WARN("AssetHandler: failed to open reader for '{0}'",
                     metadata.compiled_path.string());
    return reader;
}

uint64_t AssetHandler::get_file_mtime_ns(const Path& path)
{
    std::error_code ec;
    const auto ftime = Filesystem::last_write_time(path, ec);
    if (ec) return 0;
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            ftime.time_since_epoch()).count());
}

void AssetHandler::write_string(AssetBinaryWriter& w, const String& s)
{
    w.write_u32(static_cast<uint32_t>(s.size()));
    if (!s.empty())
        w.write_bytes(s.data(), s.size());
}

} // namespace Ignis
