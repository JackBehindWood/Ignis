#include "igpch.h"
#include "AssetRegistry.h"

namespace Ignis
{

    void AssetRegistry::register_asset(const AssetMetadata& metadata, const Path& source_key)
    {
        uint64_t key = static_cast<uint64_t>(metadata.ID);
        m_registry[key] = metadata;
        const String& index_key = source_key.empty()
            ? metadata.source_path.string()
            : source_key.string();
        m_source_index[index_key] = key;
    }

    void AssetRegistry::remove(AssetID id)
    {
        auto it = m_registry.find(static_cast<uint64_t>(id));
        if (it == m_registry.end())
            return;

        // Erase whichever source_index entry points to this ID.
        const uint64_t raw = static_cast<uint64_t>(id);
        for (auto sit = m_source_index.begin(); sit != m_source_index.end(); ++sit)
        {
            if (sit->second == raw)
            {
                m_source_index.erase(sit);
                break;
            }
        }
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
        auto it = m_source_index.find(source_path.string());
        if (it != m_source_index.end())
            return AssetID(it->second);
        return AssetID(UUID::s_invalid);
    }

} // namespace Ignis
