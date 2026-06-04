#include "igpch.h"
#include "AssetManager.h"

#include "Handlers/AssetHandler.h"

namespace Ignis
{

AssetManager& AssetManager::get()
{
    static AssetManager s_instance;
    return s_instance;
}

void PlaceholderRegistry::register_fallback(AssetType type, SharedPtr<Asset> asset)
{
    m_fallbacks[static_cast<uint16_t>(type)] = std::move(asset);
}

SharedPtr<Asset> PlaceholderRegistry::get_fallback(AssetType type) const
{
    auto it = m_fallbacks.find(static_cast<uint16_t>(type));
    IG_CORE_ASSERT(it != m_fallbacks.end(), "No fallback registered for AssetType");
    return it->second;
}

bool PlaceholderRegistry::has_fallback(AssetType type) const
{
    return m_fallbacks.count(static_cast<uint16_t>(type)) > 0;
}

void AssetManager::init(const AssetManagerConfig& config)
{
    IG_CORE_ASSERT(!m_initialized, "AssetManager already initialized");
    m_config = config;
    if (m_config.enable_file_watcher)
    {
        m_file_watcher = IFileWatcher::create();
    }
    init_default_fallbacks();
    m_initialized = true;
}

void AssetManager::init_default_fallbacks()
{
    constexpr AssetType k_types[] = {AssetType::Mesh, AssetType::Texture2D, AssetType::Shader, AssetType::Material};
    for (AssetType t : k_types)
    {
        AssetHandler* h = AssetHandler::get(t);
        if (!h)
        {
            continue;
        }
        if (SharedPtr<Asset> fb = h->create_default_fallback())
        {
            m_placeholders.register_fallback(h->get_type(), std::move(fb));
        }
    }
}

void AssetManager::shutdown()
{
    m_file_watcher.reset();
    m_graph.clear();
    m_work_queue.ready_queue.clear();
    m_work_queue.pending_map.clear();
    m_initialized = false;
}

AssetID AssetManager::import(const Path& source_path, AssetType type, bool cache, uint64_t user_data)
{
    // Deterministic ID: hash of canonical path string + user_data
    const String key_str = source_path.string() + ":" + to_string(user_data);
    uint64_t     id_val  = std::hash<String>{}(key_str);
    if (id_val == UUID::s_invalid)
    {
        id_val = 1; // avoid the sentinel
    }

    const AssetID id(id_val);
    if (m_registry.contains(id))
    {
        return id;
    }

    AssetMetadata meta;
    meta.ID             = id;
    meta.Type           = type;
    meta.source_path    = source_path;
    meta.cache_compiled = cache;
    meta.user_data      = user_data;

    if (cache)
    {
        char hex[17];
        snprintf(hex, sizeof(hex), "%016llx", static_cast<unsigned long long>(id_val));
        meta.compiled_path = m_config.compiled_root / (source_path.stem().string() + "_" + hex + ".igasset");
    }

    m_registry.register_asset(meta);

    if (m_file_watcher)
    {
        m_file_watcher->watch(source_path, static_cast<uint64_t>(id));
    }

    return id;
}

void AssetManager::reload_all()
{
    for (const auto& [key, meta] : m_registry.get_all())
    {
        if (!meta.is_valid())
        {
            continue;
        }
        AssetHandler* handler = AssetHandler::get(meta.Type);
        if (handler)
        {
            handler->compile(meta);
        }
        flag_for_reload(meta.ID);
    }
}

size_t AssetManager::get_active_count() const
{
    return m_work_queue.ready_queue.size() + m_work_queue.pending_map.size();
}

void AssetManager::prune_cache()
{
    if (!Filesystem::exists(m_config.compiled_root))
    {
        return;
    }

    for (const auto& entry : Filesystem::directory_iterator(m_config.compiled_root))
    {
        if (entry.path().extension() != ".igasset")
        {
            continue;
        }
        bool found = false;
        for (const auto& [key, meta] : m_registry.get_all())
        {
            if (meta.compiled_path == entry.path())
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            std::error_code ec;
            Filesystem::remove(entry.path(), ec);
        }
    }
}

void AssetManager::load_deferred(AssetID id)
{
    if (static_cast<uint64_t>(id) == UUID::s_invalid)
    {
        IG_CORE_ERROR("AssetManager: attempted to load invalid asset ID");
        return;
    }

    auto [it, inserted] = m_graph.emplace(id, AssetDependencyNode{});
    if (!inserted)
    {
        IG_CORE_ERROR("AssetManager: asset already tracked (id={0})", static_cast<uint64_t>(id));
        return;
    }

    it->second.id    = id;
    it->second.state = AssetState::Discovered;
    enqueue(id);
}

AssetDependencyNode& AssetManager::get_or_insert(AssetID id)
{
    auto [it, inserted] = m_graph.emplace(id, AssetDependencyNode{});
    if (inserted)
    {
        it->second.id    = id;
        it->second.state = AssetState::Discovered;
    }
    return it->second;
}

void AssetManager::enqueue(AssetID id, bool to_front)
{
    if (to_front)
    {
        m_work_queue.ready_queue.push_front(id);
    }
    else
    {
        m_work_queue.ready_queue.push_back(id);
    }
}

void AssetManager::promote_dependents(AssetID id)
{
    auto it = m_graph.find(id);
    if (it == m_graph.end())
    {
        IG_CORE_ERROR("AssetManager: promote_dependents called for untracked id");
        return;
    }

    for (AssetID dep_id : it->second.dependents)
    {
        auto pit = m_work_queue.pending_map.find(dep_id);
        if (pit == m_work_queue.pending_map.end())
        {
            IG_CORE_ERROR("AssetManager: dependent not in pending map during promotion");
            continue;
        }
        if (--pit->second == 0)
        {
            m_work_queue.pending_map.erase(pit);
            enqueue(dep_id);
        }
    }
}

void AssetManager::cascade_hot_reload(AssetDependencyNode& node)
{
    for (AssetID dep_id : node.dependents)
    {
        auto dep_it = m_graph.find(dep_id);
        if (dep_it == m_graph.end())
        {
            continue;
        }

        AssetDependencyNode& dep = dep_it->second;
        if (dep.state != AssetState::Ready)
        {
            continue;
        }

        dep.prev_asset_ptr     = dep.asset_ptr;
        dep.asset_ptr          = nullptr;
        dep.hot_reload_pending = true;
        dep.dependency_dirty   = false;
        dep.state              = AssetState::ReadingData;
        dep.read_ctx.reset();
        enqueue(dep_id, /*to_front=*/false);
    }
}

void AssetManager::mark_failed(AssetDependencyNode& node)
{
    node.state = AssetState::Failed;
    node.read_ctx.reset();
    node.asset_ptr = nullptr;

    for (AssetID dep_id : node.dependents)
    {
        auto it = m_graph.find(dep_id);
        if (it != m_graph.end() && it->second.state != AssetState::Failed)
        {
            mark_failed(it->second);
        }
    }
}

void AssetManager::update(float max_budget_ms)
{
    if (m_file_watcher)
    {
        m_file_watcher->consume_events(*this);
    }

    const auto start      = std::chrono::high_resolution_clock::now();
    auto       elapsed_ms = [&]() -> float
    { return std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - start).count(); };

    uint32_t finalizations_this_tick = 0;

    while (!m_work_queue.ready_queue.empty())
    {
        if (elapsed_ms() >= max_budget_ms)
        {
            break;
        }

        const AssetID id = m_work_queue.ready_queue.front();
        m_work_queue.ready_queue.pop_front();

        auto it = m_graph.find(id);
        if (it == m_graph.end())
        {
            continue;
        }

        AssetDependencyNode& node = it->second;

        if (node.state == AssetState::Finalizing && finalizations_this_tick >= m_config.max_finalizations_per_tick)
        {
            m_work_queue.ready_queue.push_back(id);
            break;
        }

        switch (node.state)
        {
            case AssetState::Discovered:
                process_metadata_scan(node);
                break;
            case AssetState::ReadingMetadata:
                process_graph_expansion(node);
                break;
            case AssetState::ReadingData:
                process_stream_tick(node);
                break;
            case AssetState::Finalizing:
                process_finalize(node);
                ++finalizations_this_tick;
                break;
            case AssetState::LoadingDependencies:
                IG_CORE_ASSERT(false, "LoadingDependencies node in ready_queue");
                break;
            default:
                break;
        }
    }
}

void AssetManager::process_metadata_scan(AssetDependencyNode& node)
{
    const AssetMetadata* meta = m_registry.get(node.id);
    if (!meta || !meta->is_valid())
    {
        IG_CORE_ERROR("AssetManager: metadata not found for asset {0}", static_cast<uint64_t>(node.id));
        mark_failed(node);
        return;
    }

    node.type = meta->Type;

    auto try_read_header = [&](const Path& bin_path) -> bool
    {
        BinaryReader raw(bin_path);
        if (!raw.is_open())
        {
            return false;
        }

        uint8_t magic[4];
        raw.read_bytes(magic, 4);
        const uint8_t version = raw.read_u8();
        if (version < 2)
        {
            return false;
        }

        raw.read_u16(); // asset_type — trusted from registry
        const uint32_t dep_count = raw.read_u32();
        node.dependencies.clear();
        node.dependencies.reserve(dep_count);
        for (uint32_t i = 0; i < dep_count; ++i)
        {
            node.dependencies.push_back(AssetID(raw.read_u64()));
        }

        node.payload_size   = raw.read_u64();
        node.payload_offset = 4u + 1u + 2u + 4u + static_cast<uint64_t>(dep_count) * 8u + 8u;

        return raw.good();
    };

    // Lookup order: engine_compiled_root (pre-built, read-only) → compiled_root (project cache).
    // Engine root is checked first so engine assets don't get redundantly recompiled into the
    // project cache.
    auto resolve_binary = [&]() -> bool
    {
        if (meta->cache_compiled && !m_config.engine_compiled_root.empty())
        {
            const Path engine_bin = m_config.engine_compiled_root / meta->compiled_path.filename();
            if (try_read_header(engine_bin))
            {
                node.resolved_binary_path = engine_bin;
                return true;
            }
        }

        if (try_read_header(meta->compiled_path))
        {
            node.resolved_binary_path = meta->compiled_path;
            return true;
        }

        return false;
    };

    if (!resolve_binary())
    {
        AssetHandler* handler = AssetHandler::get(meta->Type);
        if (!handler || !handler->compile(*meta))
        {
            IG_CORE_ERROR("AssetManager: compilation failed for '{0}'", meta->source_path.string());
            mark_failed(node);
            return;
        }
        node.resolved_binary_path = meta->compiled_path;
        if (!try_read_header(node.resolved_binary_path))
        {
            IG_CORE_ERROR("AssetManager: cannot read V2 header after cook for '{0}'",
                          node.resolved_binary_path.string());
            mark_failed(node);
            return;
        }
    }

    node.state = AssetState::ReadingMetadata;
    enqueue(node.id);
}

void AssetManager::process_graph_expansion(AssetDependencyNode& node)
{
    if (node.dependency_dirty)
    {
        node.dependency_dirty = false;
        node.state            = AssetState::ReadingData;
        enqueue(node.id);
        return;
    }

    uint32_t new_pending = 0;

    for (AssetID child_id : node.dependencies)
    {
        m_cycle_visiting_set.clear();
        if (has_cycle(node.id, child_id, m_cycle_visiting_set))
        {
            IG_CORE_ERROR("AssetManager: cyclic dependency (asset {0} -> {1})", static_cast<uint64_t>(node.id),
                          static_cast<uint64_t>(child_id));
            mark_failed(node);
            return;
        }

        auto [child_it, inserted] = m_graph.emplace(child_id, AssetDependencyNode{});
        if (inserted)
        {
            child_it->second.id    = child_id;
            child_it->second.state = AssetState::Discovered;
        }

        AssetDependencyNode& child = child_it->second;

        bool already_dependent = false;
        for (AssetID d : child.dependents)
        {
            if (static_cast<uint64_t>(d) == static_cast<uint64_t>(node.id))
            {
                already_dependent = true;
                break;
            }
        }

        if (!already_dependent)
        {
            child.dependents.push_back(node.id);
        }

        if (child.state == AssetState::Failed)
        {
            mark_failed(node);
            return;
        }

        if (child.state != AssetState::Ready)
        {
            ++new_pending;
            if (inserted)
            {
                enqueue(child_id);
            }
        }
    }

    node.pending_dep_count = new_pending;

    if (new_pending == 0)
    {
        node.state = AssetState::ReadingData;
        enqueue(node.id);
    }
    else
    {
        node.state                        = AssetState::LoadingDependencies;
        m_work_queue.pending_map[node.id] = new_pending;
    }
}

void AssetManager::process_stream_tick(AssetDependencyNode& node)
{
    const AssetMetadata* meta = m_registry.get(node.id);
    if (!meta)
    {
        mark_failed(node);
        return;
    }

    if (!node.read_ctx)
    {
        IG_CORE_ASSERT(node.payload_size > 0 && node.payload_offset > 0,
                       "payload_size/offset not set — metadata scan must run first");

        node.read_ctx = create_unique<StreamingReadContext>();
        auto& ctx     = *node.read_ctx;

        ctx.stream.open(node.resolved_binary_path);
        if (!ctx.stream.is_open())
        {
            IG_CORE_ERROR("AssetManager: failed to open '{0}' for streaming", node.resolved_binary_path.string());
            node.read_ctx.reset();
            mark_failed(node);
            return;
        }

        ctx.stream.seekg(node.payload_offset);
        ctx.total_bytes = node.payload_size;
        ctx.staging.reserve(node.payload_size);
    }

    auto&          ctx       = *node.read_ctx;
    const uint64_t remaining = ctx.total_bytes - ctx.bytes_consumed;
    const uint64_t chunk     = std::min(remaining, static_cast<uint64_t>(m_config.stream_chunk_bytes));

    const size_t old_size = ctx.staging.size();
    ctx.staging.resize(old_size + chunk);
    const size_t actually_read = ctx.stream.read_chunk(ctx.staging.data() + old_size, chunk);
    ctx.bytes_consumed += static_cast<uint64_t>(actually_read);

    if (ctx.bytes_consumed < ctx.total_bytes)
    {
        enqueue(node.id);
    }
    else
    {
        ctx.stream.close();
        node.state = AssetState::Finalizing;
        enqueue(node.id);
    }
}

void AssetManager::process_finalize(AssetDependencyNode& node)
{
    const AssetMetadata* meta = m_registry.get(node.id);
    if (!meta)
    {
        mark_failed(node);
        return;
    }

    AssetHandler* handler = AssetHandler::get(meta->Type);
    if (!handler)
    {
        IG_CORE_ERROR("AssetManager: no handler for type {0} (asset {1})", static_cast<uint16_t>(meta->Type),
                      static_cast<uint64_t>(node.id));
        mark_failed(node);
        return;
    }

    SharedPtr<Asset> asset;
    if (node.read_ctx && !node.read_ctx->staging.empty())
    {
        asset = handler->load_from_bytes(*meta, node.read_ctx->staging);
    }
    else
    {
        asset = handler->load(*meta);
    }

    node.read_ctx.reset();

    if (!asset)
    {
        mark_failed(node);
        return;
    }

    node.asset_ptr = asset;
    node.state     = AssetState::Ready;

    if (node.hot_reload_pending)
    {
        node.prev_asset_ptr     = nullptr;
        node.hot_reload_pending = false;
        for (const auto& [token, cb] : m_reload_subscribers)
        {
            cb(node.id);
        }
        cascade_hot_reload(node);
    }

    promote_dependents(node.id);
}

bool AssetManager::has_cycle(AssetID root, AssetID candidate, UnorderedSet<AssetID>& visiting)
{
    if (static_cast<uint64_t>(candidate) == static_cast<uint64_t>(root))
    {
        return true;
    }
    if (visiting.count(candidate))
    {
        return true;
    }

    auto it = m_graph.find(candidate);
    if (it == m_graph.end())
    {
        return false;
    }

    visiting.insert(candidate);
    for (AssetID dep : it->second.dependencies)
    {
        if (has_cycle(root, dep, visiting))
        {
            return true;
        }
    }
    visiting.erase(candidate);
    return false;
}

AssetState AssetManager::get_state(AssetID id) const
{
    auto it = m_graph.find(id);
    return it != m_graph.end() ? it->second.state : AssetState::Unloaded;
}

AssetMetadata AssetManager::get_metadata(AssetID id) const
{
    const AssetMetadata* meta = m_registry.get(id);
    return meta ? *meta : AssetMetadata{};
}

bool AssetManager::is_ready(AssetID id) const
{
    return get_state(id) == AssetState::Ready;
}

SharedPtr<Asset> AssetManager::get_asset(AssetID id) const
{
    auto it = m_graph.find(id);
    if (it == m_graph.end())
    {
        return nullptr;
    }
    const auto& node = it->second;
    if (node.asset_ptr)
    {
        return node.asset_ptr;
    }
    return node.prev_asset_ptr;
}

void AssetManager::unload(AssetID id)
{
    auto it = m_graph.find(id);
    if (it == m_graph.end())
    {
        return;
    }

    if (m_file_watcher)
    {
        m_file_watcher->unwatch(static_cast<uint64_t>(id));
    }

    it->second.asset_ptr = nullptr;
    it->second.read_ctx.reset();
    m_graph.erase(it);

    auto& rq = m_work_queue.ready_queue;
    rq.erase(std::remove_if(rq.begin(), rq.end(),
                            [id](AssetID q) { return static_cast<uint64_t>(q) == static_cast<uint64_t>(id); }),
             rq.end());

    m_work_queue.pending_map.erase(id);
}

void AssetManager::flag_for_reload(AssetID id)
{
    auto it = m_graph.find(id);
    if (it == m_graph.end())
    {
        get_or_insert(id);
        enqueue(id);
        return;
    }

    AssetDependencyNode& node = it->second;

    if (node.state == AssetState::Ready)
    {
        node.prev_asset_ptr     = node.asset_ptr;
        node.asset_ptr          = nullptr;
        node.hot_reload_pending = true;
        node.read_ctx.reset();

        if (node.dependency_dirty)
        {
            node.dependency_dirty = false;
            node.state            = AssetState::ReadingData;
        }
        else
        {
            node.state = AssetState::Discovered;
        }

        enqueue(id, /*to_front=*/true);
    }
    else if (node.state == AssetState::Failed)
    {
        node.state            = AssetState::Discovered;
        node.dependency_dirty = false;
        enqueue(id);
    }
}

uint32_t AssetManager::add_reload_callback(void (*callback)(AssetID))
{
    const uint32_t token        = m_next_reload_token++;
    m_reload_subscribers[token] = callback;
    return token;
}

void AssetManager::remove_reload_callback(uint32_t token)
{
    m_reload_subscribers.erase(token);
}

void AssetManager::register_fallback(AssetType type, SharedPtr<Asset> asset)
{
    m_placeholders.register_fallback(type, std::move(asset));
}

SharedPtr<Asset> AssetManager::get_fallback(AssetType type) const
{
    return m_placeholders.get_fallback(type);
}

} // namespace Ignis
