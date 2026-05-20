#include "igpch.h"
#include "AssetCompiler.h"
#include "AssetShaderCompiler.h"

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

        size_t size = src.get_size(); 
        out_bytes.resize(size);
        src.read_bytes(out_bytes.data(), size);
        
        return true;
    }

    AssetBinaryWriter AssetCompiler::open_writer(const AssetMetadata& metadata, const AssetBlobHeader& header) const
    {
        return AssetBinaryWriter::open(metadata, header);
    }


    // IGAS v1 — lightweight recipe: source path + entry points + defines (no bytecode).
    static constexpr AssetBlobHeader k_recipe_header = {{'I', 'G', 'A', 'S'}, 1};

    static void write_str(AssetBinaryWriter& w, const String& s)
    {
        w.write_u32(static_cast<uint32_t>(s.size()));
        if (!s.empty())
            w.write_bytes(s.data(), s.size());
    }

    bool AssetShaderCompiler::compile(const AssetMetadata& metadata)
    {
        AssetBinaryWriter w = open_writer(metadata, k_recipe_header);
        if (!w.is_open())
            return false;

        write_str(w, metadata.source_path.string());
        write_str(w, "VSMain");
        write_str(w, "PSMain");
        w.write_u32(0); // num_defines — reserved for future macro support

        return w.good();
    }

} // namespace Ignis