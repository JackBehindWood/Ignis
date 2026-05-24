#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Asset/AssetBinaryStream.h"
#include "Ignis/Foundation/SharedPtr.h"

namespace Ignis
{

class AssetHandler
{
public:
    virtual ~AssetHandler() = default;

    virtual bool             compile(const AssetMetadata& metadata) = 0;
    virtual SharedPtr<Asset> load(const AssetMetadata& metadata) = 0;

    virtual SharedPtr<Asset> load_from_bytes(const AssetMetadata& metadata, const Vector<uint8_t>& /*bytes*/)
    {
        return load(metadata);
    }

    static AssetHandler* get(AssetType type);

protected:
    bool read_source_text (const AssetMetadata& metadata, String& out_text) const;
    bool read_source_bytes(const AssetMetadata& metadata, Vector<uint8_t>& out) const;
    bool ensure_output_dir(const AssetMetadata& metadata) const;

    AssetBinaryWriter open_writer(const AssetMetadata& metadata, const AssetBlobHeader& header, const Vector<AssetID>& deps = {}) const;
    AssetBinaryReader open_reader(const AssetMetadata& metadata, const AssetBlobHeader& header) const;

public:
    static void write_string(AssetBinaryWriter& w, const String& s);

    template<typename R>
    static String read_string(R& r)
    {
        const uint32_t len = r.read_u32();
        String s(len, '\0');
        if (len > 0) 
        {
            r.read_bytes(s.data(), len);
        }
        return s;
    }

    static uint64_t get_file_mtime_ns(const Path& path);
};

} // namespace Ignis
