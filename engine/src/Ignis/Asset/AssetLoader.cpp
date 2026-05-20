#include "igpch.h"
#include "AssetLoader.h"

namespace Ignis
{

    AssetBinaryReader AssetLoader::open_reader(const AssetMetadata& metadata, const AssetBlobHeader& header) const
    {
        AssetBinaryReader reader = AssetBinaryReader::open(metadata, header);
        
        if (!reader.is_open())
        {
            //TODO: add more!
            IG_CORE_WARN("AssetLoader: Failed to open reader for asset {0}", metadata.compiled_path.string());
        }

        return reader;
    }

}