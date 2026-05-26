#pragma once

#include <Ignis.h>
#include "Project/IProjectObserver.h"

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

    void set_engine_root(const Path& root);

    const Path& engine_root() const
    {
        return m_engine_root;
    }
    const Path& project_root() const
    {
        return m_project_root;
    }

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

    void load_deferred(AssetID id);

    // --- Delegated AssetManager API (sole editor gateway) ---
    void          update(float max_budget_ms);
    void          reload_all();
    uint32_t      add_reload_callback(void (*callback)(AssetID));
    void          remove_reload_callback(uint32_t token);
    AssetMetadata get_metadata(AssetID id) const;

private:
    EditorAssetManager() = default;

    void set_project_root(const Path& root, const String& asset_source_dir = "assets");

    AssetID import(const Path& absolute_source, AssetType type);

    Path m_engine_root;
    Path m_engine_asset_root;
    Path m_project_root;
    Path m_project_asset_root;
};

} // namespace Ignis
