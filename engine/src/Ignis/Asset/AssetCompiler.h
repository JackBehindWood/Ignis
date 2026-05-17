#pragma once

#include "Asset.h"

#include "AssetBinaryStream.h"

namespace Ignis
{
    class AssetCompiler
    {
    public:
        virtual ~AssetCompiler() = default;
        virtual bool compile(const AssetMetadata& metadata) = 0;

    protected:
        bool read_source_text(const AssetMetadata& metadata, String& out_text) const;
        bool read_source_bytes(const AssetMetadata& metadata, Vector<uint8_t>& out_bytes) const;

        AssetBinaryWriter open_writer(const AssetMetadata& metadata, const AssetBlobHeader& header) const;
    };

} // namespace Ignis