#pragma once

#include "Asset.h"
#include "AssetRegistry.h"
#include "Handlers/AssetHandler.h"
#include "Ignis/Core/IFileWatcher.h"
#include "Ignis/Foundation/SharedPtr.h"
#include "Ignis/Foundation/UniquePtr.h"
#include "Ignis/Foundation/Vector.h"
#include "Ignis/Foundation/Deque.h"
#include "Ignis/Foundation/UnorderedMap.h"
#include "Ignis/Foundation/UnorderedSet.h"
#include "Ignis/Foundation/BinaryStream.h"
#include "Ignis/Foundation/Filesystem.h"

namespace Ignis
{

enum class AssetState : uint8_t
{
    Unloaded,
    Discovered,
    ReadingMetadata,
    LoadingDependencies,
    ReadingData,
    Finalizing,
    Ready,
    Failed
};

struct StreamingReadContext
{
    BinaryReader    stream;
    uint64_t        total_bytes    = 0;
    uint64_t        bytes_consumed = 0;
    Vector<uint8_t> staging;
};

struct AssetDependencyNode
{
    AssetID    id;
    AssetType  type  = AssetType::None;
    AssetState state = AssetState::Unloaded;

    Vector<AssetID> dependencies;
    Vector<AssetID> dependents;
    uint32_t        pending_dep_count = 0;

    SharedPtr<Asset> asset_ptr;
    SharedPtr<Asset> prev_asset_ptr;

    UniquePtr<StreamingReadContext> read_ctx;

    uint64_t payload_offset = 0;
    uint64_t payload_size   = 0;

    bool hot_reload_pending = false;
    bool dependency_dirty   = false;
};

struct AssetWorkQueue
{
    Deque<AssetID>                  ready_queue;
    UnorderedMap<AssetID, uint32_t> pending_map;
};

class PlaceholderRegistry
{
public:
    void             register_fallback(AssetType type, SharedPtr<Asset> asset);
    SharedPtr<Asset> get_fallback(AssetType type) const;
    bool             has_fallback(AssetType type) const;

private:
    UnorderedMap<uint16_t, SharedPtr<Asset>> m_fallbacks;
};

struct AssetManagerConfig
{
    float    default_budget_ms          = 2.0f;
    uint32_t stream_chunk_bytes         = 65536;
    uint32_t max_finalizations_per_tick = 2;
    bool     enable_file_watcher        = true;
    Path     compiled_root              = "cache/";
};

class AssetManager
{
public:
    static AssetManager& get();

    void init(const AssetManagerConfig& config = {});
    void shutdown();

    AssetID import(const Path& source_path, AssetType type, bool cache = false, uint64_t user_data = 0);

    void load_deferred(AssetID id);
    void update(float max_budget_ms);

    AssetState    get_state(AssetID id) const;
    AssetMetadata get_metadata(AssetID id) const;
    bool          is_ready(AssetID id) const;

    SharedPtr<Asset> get_asset(AssetID id) const;

    template <typename T>
    SharedPtr<T> get_asset_as(AssetID id) const
    {
        return static_pointer_cast<T>(get_asset(id));
    }

    void unload(AssetID id);
    void flag_for_reload(AssetID id);
    void reload_all();
    void prune_cache();

    template <typename T>
    SharedPtr<T> load_sync(AssetID id, float timeout_ms = 1000.0f)
    {
        if (get_state(id) == AssetState::Unloaded)
        {
            load_deferred(id);
        }
        while (get_state(id) != AssetState::Ready && get_state(id) != AssetState::Failed)
        {
            update(timeout_ms);
        }
        return get_asset_as<T>(id);
    }

    uint32_t add_reload_callback(void (*callback)(AssetID));
    void     remove_reload_callback(uint32_t token);

    void             register_fallback(AssetType type, SharedPtr<Asset> asset);
    SharedPtr<Asset> get_fallback(AssetType type) const;

    AssetRegistry& registry()
    {
        return m_registry;
    }
    const AssetRegistry& registry() const
    {
        return m_registry;
    }

private:
    AssetDependencyNode& get_or_insert(AssetID id);
    void                 enqueue(AssetID id, bool to_front = false);
    void                 promote_dependents(AssetID id);
    void                 cascade_hot_reload(AssetDependencyNode& node);

    void process_metadata_scan(AssetDependencyNode& node);
    void process_graph_expansion(AssetDependencyNode& node);
    void process_stream_tick(AssetDependencyNode& node);
    void process_finalize(AssetDependencyNode& node);

    void init_default_fallbacks();

    bool has_cycle(AssetID root, AssetID candidate, UnorderedSet<AssetID>& visiting);
    void mark_failed(AssetDependencyNode& node);

    UnorderedMap<AssetID, AssetDependencyNode> m_graph;
    AssetWorkQueue                             m_work_queue;
    PlaceholderRegistry                        m_placeholders;
    UniquePtr<IFileWatcher>                    m_file_watcher;
    AssetRegistry                              m_registry;
    AssetManagerConfig                         m_config;
    UnorderedMap<uint32_t, void (*)(AssetID)>  m_reload_subscribers;
    uint32_t                                   m_next_reload_token = 1;
    UnorderedSet<AssetID>                      m_cycle_visiting_set;
    bool                                       m_initialized = false;
};

} // namespace Ignis
