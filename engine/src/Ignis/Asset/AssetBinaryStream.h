#pragma once

#include "Asset.h"
#include "Ignis/Foundation/BinaryStream.h"

namespace Ignis
{

// Identity header written at the start of every .igasset file.
struct AssetBlobHeader
{
    uint8_t magic[4];
    uint8_t version;
};

// ---------------------------------------------------------------------------
// AssetBinaryWriter
// ---------------------------------------------------------------------------
// Writes a .igasset binary file. Construct via open(). Check is_open() before use.
// Respects metadata.cache_compiled: open() returns a not-open writer when caching is off.
struct AssetBinaryWriter
{
    // Opens the output file and writes the .igasset header.
    // Returns a not-open writer when cache_compiled is false or the file can't be created.
    static AssetBinaryWriter open(const AssetMetadata& metadata, const AssetBlobHeader& header);

    bool is_open() const { return m_writer.is_open(); }
    bool good()    const { return m_writer.good(); }

    void write_u8   (uint8_t  v)                    { m_writer.write_u8(v);             }
    void write_u32  (uint32_t v)                    { m_writer.write_u32(v);            }
    void write_bytes(const void* data, size_t size) { m_writer.write_bytes(data, size); }

    AssetBinaryWriter()                              = default;
    AssetBinaryWriter(AssetBinaryWriter&&)            = default;
    AssetBinaryWriter& operator=(AssetBinaryWriter&&) = default;

private:
    BinaryWriter m_writer;
};

// ---------------------------------------------------------------------------
// AssetBinaryReader
// ---------------------------------------------------------------------------
// Reads and validates a .igasset binary file. Construct via open(). Check is_open() before use.
struct AssetBinaryReader
{
    // Opens the compiled file and validates the .igasset header.
    // Returns a not-open reader when the file is missing or the header is invalid.
    static AssetBinaryReader open(const AssetMetadata& metadata, const AssetBlobHeader& header);

    bool is_open() const { return m_reader.is_open(); }
    bool good()    const { return m_reader.good(); }

    uint8_t  read_u8   ()                        { return m_reader.read_u8();           }
    uint32_t read_u32  ()                        { return m_reader.read_u32();          }
    void     read_bytes(void* data, size_t size) { m_reader.read_bytes(data, size);     }

    AssetBinaryReader()                              = default;
    AssetBinaryReader(AssetBinaryReader&&)            = default;
    AssetBinaryReader& operator=(AssetBinaryReader&&) = default;

private:
    BinaryReader m_reader;
};

} // namespace Ignis