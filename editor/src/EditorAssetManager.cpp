#include "EditorAssetManager.h"
#include "Ignis/Asset/AssetManager.h"

namespace Ignis
{

static constexpr const char* k_engine_prefix     = "engine:";
static constexpr size_t      k_engine_prefix_len = 7;

// --- IProjectObserver ---

void EditorAssetManager::on_project_opened(const ProjectContext& ctx)
{
    set_project_root(ctx.descriptor.root, ctx.descriptor.asset_source_dir);

    AssetManager::get().shutdown();

    AssetManagerConfig cfg;
    if (ctx.in_memory)
    {
        cfg.compiled_root        = m_engine_root / "cache";
        cfg.engine_compiled_root = m_engine_root / "cache";
    }
    else
    {
        cfg.compiled_root        = ctx.compiled_cache_abs;
        cfg.engine_compiled_root = m_engine_root / "cache";
    }
    AssetManager::get().init(cfg);
}

void EditorAssetManager::on_project_closed()
{
    AssetManager::get().shutdown();
    m_project_root       = Path{};
    m_project_asset_root = Path{};
}

// --- Setup ---

void EditorAssetManager::set_engine_root(const Path& root)
{
    m_engine_root       = root;
    m_engine_asset_root = root / "assets";
}

void EditorAssetManager::set_project_root(const Path& root, const String& asset_source_dir)
{
    m_project_root       = root;
    m_project_asset_root = root.empty() ? Path{} : (root / asset_source_dir);
}

// --- Import API ---

AssetID EditorAssetManager::import_engine_asset(const Path& relative, AssetType type)
{
    return import(m_engine_asset_root / relative, type);
}

AssetID EditorAssetManager::import_project_asset(const Path& relative, AssetType type)
{
    return import(m_project_asset_root / relative, type);
}

AssetID EditorAssetManager::resolve_reference(const String& qualified_ref, AssetType type)
{
    if (qualified_ref.compare(0, k_engine_prefix_len, k_engine_prefix) == 0)
    {
        return import_engine_asset(Path(qualified_ref.substr(k_engine_prefix_len)), type);
    }
    return import_project_asset(Path(qualified_ref), type);
}

AssetID EditorAssetManager::import_texture(const Path& filename)
{
    return import_project_asset(Path("textures") / filename, AssetType::Texture2D);
}

Pair<AssetID, AssetID> EditorAssetManager::import_shader(const Path& filename)
{
    const Path source = m_project_asset_root / "shaders" / filename;
    if (!Filesystem::exists(source))
    {
        IG_ERROR("EditorAssetManager: source file not found: {0}", source.string());
        const AssetID invalid(UUID::s_invalid);
        return {invalid, invalid};
    }
    const AssetID vs_id = AssetManager::get().import(source, AssetType::Shader, true, (uint64_t)GRIShaderStage::Vertex);
    const AssetID ps_id = AssetManager::get().import(source, AssetType::Shader, true, (uint64_t)GRIShaderStage::Pixel);
    return {vs_id, ps_id};
}

AssetID EditorAssetManager::import_mesh(const Path& filename)
{
    return import_project_asset(Path("meshes") / filename, AssetType::Mesh);
}

AssetID EditorAssetManager::import_material(const Path& filename)
{
    return import_project_asset(Path("materials") / filename, AssetType::Material);
}

SharedPtr<AssetTexture2D> EditorAssetManager::load_texture(AssetID id)
{
    return AssetManager::get().load_sync<AssetTexture2D>(id);
}

SharedPtr<AssetShader> EditorAssetManager::load_shader(AssetID id)
{
    return AssetManager::get().load_sync<AssetShader>(id);
}

SharedPtr<AssetMesh> EditorAssetManager::load_mesh(AssetID id)
{
    return AssetManager::get().load_sync<AssetMesh>(id);
}

SharedPtr<AssetMaterial> EditorAssetManager::load_material(AssetID id)
{
    return AssetManager::get().load_sync<AssetMaterial>(id);
}

void EditorAssetManager::load_deferred(AssetID id)
{
    AssetManager::get().load_deferred(id);
}

// --- Delegated API ---

void EditorAssetManager::update(float max_budget_ms)
{
    AssetManager::get().update(max_budget_ms);
}

void EditorAssetManager::reload_all()
{
    AssetManager::get().reload_all();
}

uint32_t EditorAssetManager::add_reload_callback(void (*callback)(AssetID))
{
    return AssetManager::get().add_reload_callback(callback);
}

void EditorAssetManager::remove_reload_callback(uint32_t token)
{
    AssetManager::get().remove_reload_callback(token);
}

AssetMetadata EditorAssetManager::get_metadata(AssetID id) const
{
    return AssetManager::get().get_metadata(id);
}

// --- Private ---

AssetID EditorAssetManager::import(const Path& absolute_source, AssetType type)
{
    if (!Filesystem::exists(absolute_source))
    {
        IG_ERROR("EditorAssetManager: source file not found: {0}", absolute_source.string());
        return AssetID(UUID::s_invalid);
    }
    return AssetManager::get().import(absolute_source, type, /*cache_compiled=*/true);
}

} // namespace Ignis
