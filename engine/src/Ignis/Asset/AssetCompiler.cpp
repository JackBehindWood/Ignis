#include "igpch.h"
#include "AssetCompiler.h"

namespace Ignis
{

    bool AssetCompiler::read_source_text(const AssetMetadata& metadata, String& out_text) const
    {
        if (!Filesystem::exists(metadata.source_path))
        {
            IG_CORE_ERROR("AssetCompiler: source not found: {0}", metadata.source_path.string());
            return false;
        }

        BinaryReader src(metadata.source_path);
        if (!src.is_open())
        {
            IG_CORE_ERROR("AssetCompiler: failed to open source file: {0}", metadata.source_path.string());
            return false;
        }

        out_text = src.read_all_text();
        return true;
    }

    bool AssetCompiler::read_source_bytes(const AssetMetadata& metadata, Vector<uint8_t>& out_bytes) const
    {
        if (!Filesystem::exists(metadata.source_path))
        {
            IG_CORE_ERROR("AssetCompiler: source not found: {0}", metadata.source_path.string());
            return false;
        }

        BinaryReader src(metadata.source_path);
        if (!src.is_open())
        {
            IG_CORE_ERROR("AssetCompiler: failed to open source file: {0}", metadata.source_path.string());
            return false;
        }

        // Assuming your BinaryReader has a way to get the file size
        size_t size = src.get_size(); 
        out_bytes.resize(size);
        src.read_bytes(out_bytes.data(), size);
        
        return true;
    }

    AssetBinaryWriter AssetCompiler::open_writer(const AssetMetadata& metadata, const AssetBlobHeader& header) const
    {
        return AssetBinaryWriter::open(metadata, header);
    }

}