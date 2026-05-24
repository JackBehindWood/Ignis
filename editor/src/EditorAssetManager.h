#pragma once

#include <Ignis.h>

namespace Ignis
{

    class EditorAssetManager
    {
    public:
        static EditorAssetManager& get()
        {
            static EditorAssetManager instance;
            return instance;
        }

        void        set_root(const Path& root);
        const Path& root()        const { return m_root; }
        Path        assets_dir()  const { return m_root / "assets"; }
        Path        cache_dir()   const { return m_root / "cache"; }
        Path        build_dir()   const { return m_root / "build"; }

        AssetID import_texture(const Path& filename);
        Pair<AssetID, AssetID> import_shader(const Path& filename);
        AssetID import_mesh(const Path& filename);
        AssetID import_material(const Path& filename);

        SharedPtr<AssetTexture2D> load_texture(AssetID id);
        SharedPtr<AssetShader>  load_shader(AssetID id);
        SharedPtr<AssetMesh>     load_mesh(AssetID id);
        SharedPtr<AssetMaterial> load_material(AssetID id);

        // Reload all assets (recompile + evict cache). Bound to F5 in EditorLayer.
        void reload_all() { AssetManager::get().reload_all(); }

    private:
        EditorAssetManager();

        // Resolves and validates a source path, then delegates to AssetManager::import.
        AssetID import(const Path& relative_path, AssetType type);

        Path m_root; // e.g. <exe_dir>/resources
    };

} // namespace Ignis
