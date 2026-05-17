#include "igpch.h"
#include "AssetBinaryStream.h"

namespace Ignis
{

// ---------------------------------------------------------------------------
// AssetBinaryWriter
// ---------------------------------------------------------------------------

AssetBinaryWriter AssetBinaryWriter::open(
    const AssetMetadata& metadata, const AssetBlobHeader& header)
{
    AssetBinaryWriter result;

    if (!metadata.cache_compiled)
        return result; // not-open; caching disabled

    Filesystem::create_directories(metadata.compiled_path.parent_path());

    result.m_writer = BinaryWriter(metadata.compiled_path);
    if (!result.m_writer.is_open())
    {
        IG_CORE_ERROR("AssetBinaryWriter: failed to open: {0}", metadata.compiled_path.string());
        return result; // not-open; file error
    }

    result.m_writer.write_bytes(header.magic, 4);
    result.m_writer.write_u8(header.version);
    return result;
}

// ---------------------------------------------------------------------------
// AssetBinaryReader
// ---------------------------------------------------------------------------

AssetBinaryReader AssetBinaryReader::open(
    const AssetMetadata& metadata, const AssetBlobHeader& header)
{
    AssetBinaryReader result;
    result.m_reader = BinaryReader(metadata.compiled_path);

    if (!result.m_reader.is_open())
    {
        IG_CORE_ERROR("AssetBinaryReader: compiled file not found: {0}", metadata.compiled_path.string());
        return result; // not-open
    }

    uint8_t file_magic[4];
    result.m_reader.read_bytes(file_magic, 4);
    if (file_magic[0] != header.magic[0] || file_magic[1] != header.magic[1] ||
        file_magic[2] != header.magic[2] || file_magic[3] != header.magic[3])
    {
        IG_CORE_ERROR("AssetBinaryReader: bad magic in {0}", metadata.compiled_path.string());
        result.m_reader = BinaryReader(); // close
        return result;
    }

    const uint8_t file_version = result.m_reader.read_u8();
    if (file_version != header.version)
    {
        IG_CORE_ERROR("AssetBinaryReader: version mismatch (expected {0}, got {1}) in {2}",
                      header.version, file_version, metadata.compiled_path.string());
        result.m_reader = BinaryReader(); // close
        return result;
    }

    return result;
}

} // namespace Ignis