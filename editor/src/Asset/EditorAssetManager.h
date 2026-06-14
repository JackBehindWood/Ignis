#pragma once

#include <Ignis.h>
#include "Project/IProjectObserver.h"
#include "IAssetEventObserver.h"
#include "IBLBaker.h"

namespace Ignis
{

class EditorAssetManager : public IProjectObserver
{
public:
    static EditorAssetManager& get()
    {
        static EditorAssetManager instance;
        return instance;
    }

    // IProjectObserver
    void on_project_opened(const ProjectContext& ctx) override;
    void on_project_closed() override;

    void set_engine_root(const Path& root, const Path& engine_shaders);

    const Path& engine_root() const
    {
        return m_engine_root;
    }
    const Path& project_root() const
    {
        return m_project_root;
    }

    void          ensure_brdf_lut();
    IBLBakeResult cook_ibl_environment(const Path& equirect_abs_path);

    // --- Asset pipeline ---
    AssetID import_engine_asset(const Path& relative, AssetType type);
    AssetID import_project_asset(const Path& relative, AssetType type);

    AssetID resolve_reference(const String& qualified_ref, AssetType type);

    AssetID                import_texture(const Path& filename);
    Pair<AssetID, AssetID> import_shader(const Path& filename);
    AssetID                import_mesh(const Path& filename);
    AssetID                import_material(const Path& filename);

    SharedPtr<AssetTexture2D> load_texture(AssetID id);
    SharedPtr<AssetShader>    load_shader(AssetID id);
    SharedPtr<AssetMesh>      load_mesh(AssetID id);
    SharedPtr<AssetMaterial>  load_material(AssetID id);

    SharedPtr<AssetTexture2D> try_get_texture(AssetID id) const;
    SharedPtr<AssetMesh>      try_get_mesh(AssetID id) const;
    SharedPtr<AssetMaterial>  try_get_material(AssetID id) const;
    AssetID                   import_asset_at(const Path& absolute_path, AssetType type);

    void load_deferred(AssetID id);

    // --- Delegated AssetManager API (sole editor gateway) ---
    void          update(float max_budget_ms);
    void          reload_all();
    size_t        get_active_count() const;
    uint32_t      add_reload_callback(void (*callback)(AssetID));
    void          remove_reload_callback(uint32_t token);
    AssetMetadata get_metadata(AssetID id) const;
    Path          try_get_source_path(AssetID id) const
    {
        return get_metadata(id).source_path;
    }

    // --- Asset event observers ---
    // All notifications fire synchronously on the main thread at the call site
    // of the triggering import_* or notify_* method — no locking needed.
    void add_observer(IAssetEventObserver* obs);
    void remove_observer(IAssetEventObserver* obs);
    void notify_imported(const Path& abs_path);
    void notify_renamed(const Path& old_path, const Path& new_path);
    void notify_deleted(const Path& abs_path);
    void notify_directory_changed();

private:
    EditorAssetManager() = default;

    void set_project_root(const Path& root, const String& asset_source_dir = "assets");

    AssetID import(const Path& absolute_source, AssetType type);

    Path                         m_engine_root;
    Path                         m_engine_asset_root;
    Path                         m_project_root;
    Path                         m_project_asset_root;
    Vector<IAssetEventObserver*> m_observers;
    IBLBaker                     m_ibl_baker;
};

} // namespace Ignis
