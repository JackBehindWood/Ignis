#include "igpch.h"
#include "AssetManager.h"

#include "AssetMesh.h"
#include "AssetTexture2D.h"
#include "AssetMaterial.h"
#include "Handlers/AssetHandler.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/RenderTexture2D.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"

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

void FileWatcher::watch(const Path& source_path, AssetID id)
{
    for (const auto& e : m_entries)
        if (static_cast<uint64_t>(e.id) == static_cast<uint64_t>(id)) return;

    WatchEntry entry;
    entry.source_path = source_path;
    entry.id          = id;
    if (Filesystem::exists(source_path))
        entry.last_mtime = Filesystem::last_write_time(source_path);

    m_entries.push_back(std::move(entry));
}

void FileWatcher::unwatch(AssetID id)
{
    m_entries.erase(
        std::remove_if(m_entries.begin(), m_entries.end(),
            [id](const WatchEntry& e) {
                return static_cast<uint64_t>(e.id) == static_cast<uint64_t>(id);
            }),
        m_entries.end());
}

void FileWatcher::poll_changes(AssetManager& manager)
{
    if (++m_frames_since_poll < m_poll_interval_frames) return;
    m_frames_since_poll = 0;

    for (auto& entry : m_entries)
    {
        if (!Filesystem::exists(entry.source_path)) continue;

        const auto mtime = Filesystem::last_write_time(entry.source_path);
        if (mtime != entry.last_mtime)
        {
            entry.last_mtime = mtime;
            manager.flag_for_reload(entry.id);
        }
    }
}

void AssetManager::init(const AssetManagerConfig& config)
{
    IG_CORE_ASSERT(!m_initialized, "AssetManager already initialized");
    m_config = config;
    m_file_watcher.set_poll_interval(config.watcher_poll_interval_frames);
    init_default_fallbacks();
    m_initialized = true;
}

void AssetManager::init_default_fallbacks()
{
    // Mesh fallback — unit cube, CPU-only, no GRI needed
    {
        struct CubeVert { float px,py,pz, nx,ny,nz, u,v; };
        static const CubeVert k_verts[24] = {
            {-0.5f,-0.5f, 0.5f,  0,0,1,  0,1}, { 0.5f,-0.5f, 0.5f,  0,0,1,  1,1},
            { 0.5f, 0.5f, 0.5f,  0,0,1,  1,0}, {-0.5f, 0.5f, 0.5f,  0,0,1,  0,0},
            { 0.5f,-0.5f,-0.5f,  0,0,-1, 0,1}, {-0.5f,-0.5f,-0.5f,  0,0,-1, 1,1},
            {-0.5f, 0.5f,-0.5f,  0,0,-1, 1,0}, { 0.5f, 0.5f,-0.5f,  0,0,-1, 0,0},
            { 0.5f,-0.5f, 0.5f,  1,0,0,  0,1}, { 0.5f,-0.5f,-0.5f,  1,0,0,  1,1},
            { 0.5f, 0.5f,-0.5f,  1,0,0,  1,0}, { 0.5f, 0.5f, 0.5f,  1,0,0,  0,0},
            {-0.5f,-0.5f,-0.5f, -1,0,0,  0,1}, {-0.5f,-0.5f, 0.5f, -1,0,0,  1,1},
            {-0.5f, 0.5f, 0.5f, -1,0,0,  1,0}, {-0.5f, 0.5f,-0.5f, -1,0,0,  0,0},
            {-0.5f, 0.5f, 0.5f,  0,1,0,  0,1}, { 0.5f, 0.5f, 0.5f,  0,1,0,  1,1},
            { 0.5f, 0.5f,-0.5f,  0,1,0,  1,0}, {-0.5f, 0.5f,-0.5f,  0,1,0,  0,0},
            {-0.5f,-0.5f,-0.5f,  0,-1,0, 0,1}, { 0.5f,-0.5f,-0.5f,  0,-1,0, 1,1},
            { 0.5f,-0.5f, 0.5f,  0,-1,0, 1,0}, {-0.5f,-0.5f, 0.5f,  0,-1,0, 0,0},
        };
        static const uint32_t k_idx[36] = {
             0, 1, 2,  0, 2, 3,   4, 5, 6,  4, 6, 7,
             8, 9,10,  8,10,11,  12,13,14, 12,14,15,
            16,17,18, 16,18,19,  20,21,22, 20,22,23,
        };
        Vector<uint8_t>  verts(sizeof(k_verts));
        std::memcpy(verts.data(), k_verts, sizeof(k_verts));
        Vector<uint32_t> idx(k_idx, k_idx + 36);
        m_placeholders.register_fallback(
            AssetType::Mesh,
            create_shared<AssetMesh>(UUID{UUID::s_invalid}, verts, idx, 32u));
    }

    // Texture fallback — 4×4 magenta checkerboard (requires active GRI)
    if (auto* gri = RenderSystem::get_gri())
    {
        constexpr uint32_t kW = 4, kH = 4;
        uint8_t pixels[kW * kH * 4];
        for (uint32_t row = 0; row < kH; ++row)
            for (uint32_t col = 0; col < kW; ++col)
            {
                const bool mag = ((row + col) & 1) == 0;
                uint8_t* p     = &pixels[(row * kW + col) * 4];
                p[0] = mag ? 0xFF : 0x80; p[1] = 0x00;
                p[2] = mag ? 0xFF : 0x80; p[3] = 0xFF;
            }

        GRITexture2DDesc desc;
        desc.width = kW; desc.height = kH; desc.num_mip_levels = 1;
        desc.format            = GRIPixelFormat::RGBA8Unorm;
        desc.initial_data      = pixels;
        desc.initial_data_size = sizeof(pixels);

        if (GRITexture2DPtr tex = gri->create_texture2d(desc))
        {
            auto rt = create_shared<RenderTexture2D>(std::move(tex), kW, kH, desc.format);
            m_placeholders.register_fallback(
                AssetType::Texture2D,
                create_shared<AssetTexture2D>(UUID{UUID::s_invalid}, std::move(rt)));
        }
    }
}

void AssetManager::shutdown()
{
    m_graph.clear();
    m_work_queue.ready_queue.clear();
    m_work_queue.pending_map.clear();
    m_initialized = false;
}

// ---------------------------------------------------------------------------
// import
// ---------------------------------------------------------------------------

AssetID AssetManager::import(const Path& source_path, AssetType type,
                              bool cache, uint64_t user_data)
{
    // Deterministic ID: hash of canonical path string + user_data
    const String key_str = source_path.string() + ":" + to_string(user_data);
    uint64_t id_val = std::hash<String>{}(key_str);
    if (id_val == UUID::s_invalid) id_val = 1; // avoid the sentinel

    const AssetID id(id_val);
    if (m_registry.contains(id))
        return id;

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
        meta.compiled_path = m_config.compiled_root /
            (source_path.stem().string() + "_" + hex + ".igasset");
    }

    m_registry.register_asset(meta);

    if (m_config.enable_file_watcher)
        m_file_watcher.watch(source_path, id);

    return id;
}

// ---------------------------------------------------------------------------
// reload_all / prune_cache
// ---------------------------------------------------------------------------

void AssetManager::reload_all()
{
    for (const auto& [key, meta] : m_registry.get_all())
    {
        if (!meta.is_valid()) continue;
        AssetHandler* handler = AssetHandler::get(meta.Type);
        if (handler) handler->compile(meta);
        flag_for_reload(meta.ID);
    }
}

void AssetManager::prune_cache()
{
    if (!Filesystem::exists(m_config.compiled_root)) return;

    for (const auto& entry : Filesystem::directory_iterator(m_config.compiled_root))
    {
        if (entry.path().extension() != ".igasset") continue;
        bool found = false;
        for (const auto& [key, meta] : m_registry.get_all())
        {
            if (meta.compiled_path == entry.path()) { found = true; break; }
        }
        if (!found)
        {
            std::error_code ec;
            Filesystem::remove(entry.path(), ec);
        }
    }
}

// ---------------------------------------------------------------------------
// Deferred load / graph management
// ---------------------------------------------------------------------------

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
        IG_CORE_ERROR("AssetManager: asset already tracked (id={0})",
                      static_cast<uint64_t>(id));
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
        m_work_queue.ready_queue.insert(m_work_queue.ready_queue.begin(), id);
    else
        m_work_queue.ready_queue.push_back(id);
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
        if (dep_it == m_graph.end()) continue;

        AssetDependencyNode& dep = dep_it->second;
        if (dep.state != AssetState::Ready) continue;

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
            mark_failed(it->second);
    }
}

// ---------------------------------------------------------------------------
// update
// ---------------------------------------------------------------------------

void AssetManager::update(float max_budget_ms)
{
    if (m_config.enable_file_watcher)
        m_file_watcher.poll_changes(*this);

    const auto start = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = [&]() -> float {
        return std::chrono::duration<float, std::milli>(
            std::chrono::high_resolution_clock::now() - start).count();
    };

    uint32_t finalizations_this_tick = 0;

    while (!m_work_queue.ready_queue.empty())
    {
        if (elapsed_ms() >= max_budget_ms) break;

        const AssetID id = m_work_queue.ready_queue.front();
        m_work_queue.ready_queue.erase(m_work_queue.ready_queue.begin());

        auto it = m_graph.find(id);
        if (it == m_graph.end()) continue;

        AssetDependencyNode& node = it->second;

        if (node.state == AssetState::Finalizing &&
            finalizations_this_tick >= m_config.max_finalizations_per_tick)
        {
            m_work_queue.ready_queue.push_back(id);
            break;
        }

        switch (node.state)
        {
            case AssetState::Discovered:      process_metadata_scan(node);   break;
            case AssetState::ReadingMetadata: process_graph_expansion(node); break;
            case AssetState::ReadingData:     process_stream_tick(node);     break;
            case AssetState::Finalizing:
                process_finalize(node);
                ++finalizations_this_tick;
                break;
            case AssetState::LoadingDependencies:
                IG_CORE_ASSERT(false, "LoadingDependencies node in ready_queue");
                break;
            default: break;
        }
    }
}

// ---------------------------------------------------------------------------
// Pipeline stages
// ---------------------------------------------------------------------------

void AssetManager::process_metadata_scan(AssetDependencyNode& node)
{
    const AssetMetadata* meta = m_registry.get(node.id);
    if (!meta || !meta->is_valid())
    {
        IG_CORE_ERROR("AssetManager: metadata not found for asset {0}",
                      static_cast<uint64_t>(node.id));
        mark_failed(node);
        return;
    }

    node.type = meta->Type;

    auto try_read_header = [&]() -> bool
    {
        BinaryReader raw(meta->compiled_path);
        if (!raw.is_open()) return false;

        uint8_t magic[4];
        raw.read_bytes(magic, 4);
        const uint8_t version = raw.read_u8();
        if (version != 2) return false;

        raw.read_u16(); // asset_type — trusted from registry
        const uint32_t dep_count = raw.read_u32();
        node.dependencies.clear();
        node.dependencies.reserve(dep_count);
        for (uint32_t i = 0; i < dep_count; ++i)
            node.dependencies.push_back(AssetID(raw.read_u64()));

        node.payload_size   = raw.read_u64();
        node.payload_offset = 4u + 1u + 2u + 4u +
                              static_cast<uint64_t>(dep_count) * 8u + 8u;

        return raw.good();
    };

    if (!try_read_header())
    {
        AssetHandler* handler = AssetHandler::get(meta->Type);
        if (!handler || !handler->compile(*meta))
        {
            IG_CORE_ERROR("AssetManager: compilation failed for '{0}'",
                          meta->source_path.string());
            mark_failed(node);
            return;
        }
        if (!try_read_header())
        {
            IG_CORE_ERROR("AssetManager: cannot read V2 header after cook for '{0}'",
                          meta->compiled_path.string());
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
        UnorderedSet<AssetID> visiting;
        if (has_cycle(node.id, child_id, visiting))
        {
            IG_CORE_ERROR("AssetManager: cyclic dependency (asset {0} -> {1})",
                          static_cast<uint64_t>(node.id),
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
            if (static_cast<uint64_t>(d) == static_cast<uint64_t>(node.id))
            { already_dependent = true; break; }

        if (!already_dependent)
            child.dependents.push_back(node.id);

        if (child.state == AssetState::Failed)
        {
            mark_failed(node);
            return;
        }

        if (child.state != AssetState::Ready)
        {
            ++new_pending;
            if (inserted) enqueue(child_id);
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
        node.state = AssetState::LoadingDependencies;
        m_work_queue.pending_map[node.id] = new_pending;
    }
}

void AssetManager::process_stream_tick(AssetDependencyNode& node)
{
    const AssetMetadata* meta = m_registry.get(node.id);
    if (!meta) { mark_failed(node); return; }

    if (!node.read_ctx)
    {
        IG_CORE_ASSERT(node.payload_size > 0 && node.payload_offset > 0,
                       "payload_size/offset not set — metadata scan must run first");

        node.read_ctx = create_unique<StreamingReadContext>();
        auto& ctx     = *node.read_ctx;

        ctx.stream.open(meta->compiled_path);
        if (!ctx.stream.is_open())
        {
            IG_CORE_ERROR("AssetManager: failed to open '{0}' for streaming",
                          meta->compiled_path.string());
            node.read_ctx.reset();
            mark_failed(node);
            return;
        }

        ctx.stream.seekg(node.payload_offset);
        ctx.total_bytes = node.payload_size;
        ctx.staging.reserve(node.payload_size);
    }

    auto& ctx                = *node.read_ctx;
    const uint64_t remaining = ctx.total_bytes - ctx.bytes_consumed;
    const uint64_t chunk     = std::min(remaining,
                                        static_cast<uint64_t>(m_config.stream_chunk_bytes));

    const size_t old_size = ctx.staging.size();
    ctx.staging.resize(old_size + chunk);
    const size_t actually_read = ctx.stream.read_chunk(ctx.staging.data() + old_size, chunk);
    ctx.bytes_consumed += static_cast<uint64_t>(actually_read);

    if (ctx.bytes_consumed < ctx.total_bytes)
        enqueue(node.id);
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
    if (!meta) { mark_failed(node); return; }

    AssetHandler* handler = AssetHandler::get(meta->Type);
    if (!handler)
    {
        IG_CORE_ERROR("AssetManager: no handler for type {0} (asset {1})",
                      static_cast<uint16_t>(meta->Type),
                      static_cast<uint64_t>(node.id));
        mark_failed(node);
        return;
    }

    SharedPtr<Asset> asset;
    if (node.read_ctx && !node.read_ctx->staging.empty())
        asset = handler->load_from_bytes(*meta, node.read_ctx->staging);
    else
        asset = handler->load(*meta);

    node.read_ctx.reset();

    if (!asset) { mark_failed(node); return; }

    node.asset_ptr = asset;
    node.state     = AssetState::Ready;

    if (node.hot_reload_pending)
    {
        node.prev_asset_ptr    = nullptr;
        node.hot_reload_pending = false;
        if (m_on_asset_reloaded)
            m_on_asset_reloaded(node.id);
        cascade_hot_reload(node);
    }

    promote_dependents(node.id);
}

// ---------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------

bool AssetManager::has_cycle(AssetID root, AssetID candidate,
                              UnorderedSet<AssetID>& visiting)
{
    if (static_cast<uint64_t>(candidate) == static_cast<uint64_t>(root)) return true;
    if (visiting.count(candidate)) return true;

    auto it = m_graph.find(candidate);
    if (it == m_graph.end()) return false;

    visiting.insert(candidate);
    for (AssetID dep : it->second.dependencies)
        if (has_cycle(root, dep, visiting)) return true;
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
    if (it == m_graph.end()) return nullptr;
    const auto& node = it->second;
    if (node.asset_ptr) return node.asset_ptr;
    return node.prev_asset_ptr;
}

void AssetManager::unload(AssetID id)
{
    auto it = m_graph.find(id);
    if (it == m_graph.end()) return;

    it->second.asset_ptr = nullptr;
    it->second.read_ctx.reset();
    m_graph.erase(it);

    auto& rq = m_work_queue.ready_queue;
    rq.erase(std::remove_if(rq.begin(), rq.end(),
                 [id](AssetID q) {
                     return static_cast<uint64_t>(q) == static_cast<uint64_t>(id);
                 }),
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
        node.prev_asset_ptr    = node.asset_ptr;
        node.asset_ptr         = nullptr;
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

void AssetManager::set_reload_callback(void (*callback)(AssetID))
{
    m_on_asset_reloaded = callback;
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
