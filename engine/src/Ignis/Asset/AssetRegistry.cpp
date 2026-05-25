#include "igpch.h"
#include "AssetRegistry.h"

namespace Ignis
{

void AssetRegistry::register_asset(const AssetMetadata& metadata, const Path& source_key)
{
    const uint64_t key = static_cast<uint64_t>(metadata.ID);
    m_registry[key]    = metadata;
    const RuntimePathId pid(source_key.empty() ? metadata.source_path : source_key);
    m_source_index[pid] = key;
}

void AssetRegistry::remove(AssetID id)
{
    auto it = m_registry.find(static_cast<uint64_t>(id));
    if (it == m_registry.end())
    {
        IG_CORE_ERROR("AssetRegistry: attempt to remove non-existent asset {0}", static_cast<uint64_t>(id));
        return;
    }

    // O(1) erase: hash the source_path from metadata rather than scanning the index.
    m_source_index.erase(RuntimePathId(it->second.source_path));
    m_registry.erase(it);
}

const AssetMetadata* AssetRegistry::get(AssetID id) const
{
    auto it = m_registry.find(static_cast<uint64_t>(id));
    return it != m_registry.end() ? &it->second : nullptr;
}

bool AssetRegistry::contains(AssetID id) const
{
    return m_registry.count(static_cast<uint64_t>(id)) > 0;
}

AssetID AssetRegistry::find_by_source(const Path& source_path) const
{
    auto it = m_source_index.find(RuntimePathId(source_path));
    return it != m_source_index.end() ? AssetID(it->second) : AssetID(UUID::s_invalid);
}
} // namespace Ignis
