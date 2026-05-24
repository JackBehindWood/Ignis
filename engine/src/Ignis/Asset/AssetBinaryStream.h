#pragma once

#include "Asset.h"
#include "Ignis/Foundation/BinaryStream.h"
#include "Ignis/Foundation/Vector.h"

namespace Ignis
{

// Per-type identity tag written at the start of every .igasset file.
struct AssetBlobHeader
{
    uint8_t magic[4];
    uint8_t version;
};

// V2 .igasset header layout (on disk):
//   magic[4]        — type-specific magic (IGAM / IGAS / IGTX / IGMT)
//   version  u8     — AssetType- specific
//   type     u16    — AssetType enum value
//   dep_count u32   — number of asset-level dependency IDs
//   dep_ids  u64×N  — AssetID values
//   payload_size u64 — byte count of payload section (filled by finalize())
//   [payload]       — type-specific data; reader/writer positioned here after open()

struct AssetBinaryWriter
{

    static AssetBinaryWriter open(const AssetMetadata& metadata, const AssetBlobHeader& header, AssetType type, const Vector<AssetID>& deps = {});

    static AssetBinaryWriter open(const Path& path, const AssetBlobHeader& header, AssetType type, const Vector<AssetID>& deps = {});

    // Seeks back and writes the real payload_size into the header placeholder.
    // Must be called after all payload has been written. Returns good().
    bool finalize();

    bool is_open() const { return m_writer.is_open(); }
    bool good()    const { return m_writer.good(); }

    void write_u8   (uint8_t  v)                    { m_writer.write_u8(v);             }
    void write_u16  (uint16_t v)                    { m_writer.write_u16(v);            }
    void write_u32  (uint32_t v)                    { m_writer.write_u32(v);            }
    void write_u64  (uint64_t v)                    { m_writer.write_u64(v);            }
    void write_bytes(const void* data, size_t size) { m_writer.write_bytes(data, size); }

    AssetBinaryWriter()                              = default;
    AssetBinaryWriter(AssetBinaryWriter&&)            = default;
    AssetBinaryWriter& operator=(AssetBinaryWriter&&) = default;

private:
    BinaryWriter   m_writer;
    BinaryWriter::StreamPos m_payload_size_pos{-1};
    BinaryWriter::StreamPos m_payload_start{-1};
};

struct AssetBinaryReader
{
    static AssetBinaryReader open(const AssetMetadata& metadata, const AssetBlobHeader& header);
    static AssetBinaryReader open(const Path& path, const AssetBlobHeader& header);

    bool is_open() const { return m_reader.is_open(); }
    bool good()    const { return m_reader.good(); }

    uint8_t  read_u8   ()  { return m_reader.read_u8(); }
    uint16_t read_u16  ()  { return m_reader.read_u16(); }
    uint32_t read_u32  ()  { return m_reader.read_u32(); }
    uint64_t read_u64  ()  { return m_reader.read_u64(); }
    void     read_bytes(void* data, size_t size) { m_reader.read_bytes(data, size); }

    AssetBinaryReader() = default;
    AssetBinaryReader(AssetBinaryReader&&) = default;
    AssetBinaryReader& operator=(AssetBinaryReader&&) = default;

private:
    BinaryReader m_reader;
};

} // namespace Ignis
