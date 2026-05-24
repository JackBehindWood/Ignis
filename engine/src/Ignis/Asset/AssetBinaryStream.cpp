#include "igpch.h"
#include "AssetBinaryStream.h"

namespace Ignis
{

static void write_v2_header(AssetBinaryWriter& result, BinaryWriter& w, const AssetBlobHeader& header, AssetType type,  const Vector<AssetID>& deps)
{
    (void)result;
    w.write_bytes(header.magic, 4);
    w.write_u8(header.version);
    w.write_u16(static_cast<uint16_t>(type));
    w.write_u32(static_cast<uint32_t>(deps.size()));
    for (const AssetID& dep_id : deps)
    {
        w.write_u64(static_cast<uint64_t>(dep_id));
    }
}

AssetBinaryWriter AssetBinaryWriter::open(const AssetMetadata& metadata, const AssetBlobHeader& header, AssetType type, const Vector<AssetID>& deps)
{
    AssetBinaryWriter result;

    if (!metadata.cache_compiled)
    {
        return result;
    }

    Filesystem::create_directories(metadata.compiled_path.parent_path());
    result.m_writer = BinaryWriter(metadata.compiled_path);
    if (!result.m_writer.is_open())
    {
        IG_CORE_ERROR("AssetBinaryWriter: failed to open: {}", metadata.compiled_path.string());
        return result;
    }

    result.m_writer.write_bytes(header.magic, 4);
    result.m_writer.write_u8(header.version);
    result.m_writer.write_u16(static_cast<uint16_t>(type));
    result.m_writer.write_u32(static_cast<uint32_t>(deps.size()));
    for (const AssetID& dep_id : deps)
    {
        result.m_writer.write_u64(static_cast<uint64_t>(dep_id));
    }

    result.m_payload_size_pos = result.m_writer.tell();
    result.m_writer.write_u64(0); // payload_size placeholder
    result.m_payload_start = result.m_writer.tell();

    return result;
}

AssetBinaryWriter AssetBinaryWriter::open(const Path& path, const AssetBlobHeader& header, AssetType type, const Vector<AssetID>& deps)
{
    AssetBinaryWriter result;
    Filesystem::create_directories(path.parent_path());
    result.m_writer = BinaryWriter(path);
    if (!result.m_writer.is_open())
    {
        IG_CORE_ERROR("AssetBinaryWriter: failed to open: {}", path.string());
        return result;
    }

    result.m_writer.write_bytes(header.magic, 4);
    result.m_writer.write_u8(header.version);
    result.m_writer.write_u16(static_cast<uint16_t>(type));
    result.m_writer.write_u32(static_cast<uint32_t>(deps.size()));
    for (const AssetID& dep_id : deps)
    {
        result.m_writer.write_u64(static_cast<uint64_t>(dep_id));
    }

    result.m_payload_size_pos = result.m_writer.tell();
    result.m_writer.write_u64(0);
    result.m_payload_start = result.m_writer.tell();

    return result;
}

bool AssetBinaryWriter::finalize()
{
    if (!m_writer.is_open() || m_payload_start < std::streampos(0))
    {
        IG_CORE_ERROR("AssetBinaryWriter: cannot finalize; invalid state");
        return false;
    }

    const std::streampos payload_end  = m_writer.tell();
    const uint64_t       payload_size = static_cast<uint64_t>(payload_end - m_payload_start);

    m_writer.seekp(m_payload_size_pos);
    m_writer.write_u64(payload_size);
    m_writer.seekp(payload_end);

    return m_writer.good();
}

static bool open_v2_reader(BinaryReader& reader, const AssetBlobHeader& header, const String& path_for_log)
{
    if (!reader.is_open())
    {
        IG_CORE_ERROR("AssetBinaryReader: file not found: {}", path_for_log);
        return false;
    }

    uint8_t file_magic[4];
    reader.read_bytes(file_magic, 4);
    if (file_magic[0] != header.magic[0] || file_magic[1] != header.magic[1] ||
        file_magic[2] != header.magic[2] || file_magic[3] != header.magic[3])
    {
        IG_CORE_ERROR("AssetBinaryReader: bad magic in {}", path_for_log);
        return false;
    }

    const uint8_t version = reader.read_u8();
    if (version != 2)
    {
        IG_CORE_WARN("AssetBinaryReader: expected version 2, got {} in '{}'; stale cache", version, path_for_log);
        return false;
    }

    // Skip: asset_type(2) + dep_count(4) + dep_ids + payload_size(8)
    reader.read_u16(); // asset_type — discard
    const uint32_t dep_count = reader.read_u32();
    for (uint32_t i = 0; i < dep_count; ++i)
        reader.read_u64();  // dep_id — discard
    reader.read_u64(); // payload_size — discard

    return reader.good();
}

AssetBinaryReader AssetBinaryReader::open(const AssetMetadata& metadata, const AssetBlobHeader& header)
{
    AssetBinaryReader result;
    result.m_reader = BinaryReader(metadata.compiled_path);
    if (!open_v2_reader(result.m_reader, header, metadata.compiled_path.string()))
    {
        result.m_reader = BinaryReader();
    }
    return result;
}

AssetBinaryReader AssetBinaryReader::open(const Path& path, const AssetBlobHeader& header)
{
    AssetBinaryReader result;
    result.m_reader = BinaryReader(path);
    if (!open_v2_reader(result.m_reader, header, path.string()))
    {
        result.m_reader = BinaryReader();
    }
    return result;
}

} // namespace Ignis
