#pragma once

#include <Ignis.h>

namespace Ignis
{

    // Editor-only layer on top of AssetManager.
    // Enforces the resources/ directory layout and provides typed import helpers.
    // When the project system arrives, call set_root() to point at the project folder.
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
        Path        assets_dir()  const { return m_root / "assets"; }   // human-editable source files
        Path        cache_dir()   const { return m_root / "cache"; }    // compiled binary .igasset files
        Path        build_dir()   const { return m_root / "build"; }    // future build outputs

        // Typed imports — filename only, no path prefix needed.
        // e.g. import_texture("rock.png")  →  resources/assets/textures/rock.png
        AssetID import_texture(const Path& filename);
        AssetID import_shader(const Path& filename);

        // Convenience typed loaders — trigger compile-on-first-load via AssetManager.
        SharedPtr<Shader> load_shader(AssetID id);

        // Reload all assets (recompile + evict cache). Bound to F5 in EditorLayer.
        void reload_all() { AssetManager::get().reload_all(); }

    private:
        EditorAssetManager();

        // Resolves and validates a source path, then delegates to AssetManager::import.
        AssetID import(const Path& relative_path, AssetType type);

        Path m_root; // e.g. <exe_dir>/resources
    };

} // namespace Ignis
