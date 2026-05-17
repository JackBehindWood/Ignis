#pragma once

#include "Asset.h"

namespace Ignis
{

    class AssetRegistry
    {
    public:
        void                    register_asset(const AssetMetadata& metadata);
        void                    remove(AssetID id);

        const AssetMetadata*    get(AssetID id) const;
        bool                    contains(AssetID id) const;

        // Returns the existing ID for a source path, or an invalid ID if not registered.
        AssetID                 find_by_source(const Path& source_path) const;

        const UnorderedMap<uint64_t, AssetMetadata>& get_all() const { return m_registry; }

    private:
        UnorderedMap<uint64_t, AssetMetadata> m_registry;
        UnorderedMap<String, uint64_t>        m_source_index; // source_path.string() → ID
    };

} // namespace Ignis
