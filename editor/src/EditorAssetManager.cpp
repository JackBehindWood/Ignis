#include "EditorAssetManager.h"

namespace Ignis
{

    // Sub-directories per asset type — single source of truth for the layout.
    static Path subdir_for(AssetType type)
    {
        switch (type)
        {
            case AssetType::Texture2D: return "textures";
            case AssetType::Shader:    return "shaders";
            case AssetType::Mesh:      return "meshes";
            default:                   return "";
        }
    }

    EditorAssetManager::EditorAssetManager()
    {
        // Default root: <working_directory>/resources
        // Override with set_root() once the project system knows the project path.
        set_root(Filesystem::current_path() / "resources");
    }

    void EditorAssetManager::set_root(const Path& root)
    {
        m_root = root;
        AssetManager::get().set_compiled_root(cache_dir());
        AssetManager::get().prune_cache();
    }

    AssetID EditorAssetManager::import_texture(const Path& filename)
    {
        return import(Path("assets/textures") / filename, AssetType::Texture2D);
    }

    AssetID EditorAssetManager::import_shader(const Path& filename)
    {
        return import(Path("assets/shaders") / filename, AssetType::Shader);
    }

    AssetID EditorAssetManager::import_mesh(const Path& filename)
    {
        return import(Path("assets/meshes") / filename, AssetType::Mesh);
    }

    SharedPtr<AssetShader> EditorAssetManager::load_shader(AssetID id)
    {
        return AssetManager::get().load_as<AssetShader>(id);
    }

    SharedPtr<AssetMesh> EditorAssetManager::load_mesh(AssetID id)
    {
        return AssetManager::get().load_as<AssetMesh>(id);
    }

    AssetID EditorAssetManager::import(const Path& relative_path, AssetType type)
    {
        Path source = m_root / relative_path;

        if (!Filesystem::exists(source))
        {
            IG_ERROR("EditorAssetManager: source file not found: {0}", source.string());
            return AssetID(UUID::s_invalid);
        }

        // Ensure the cache directory exists before the compiler writes into it.
        Path cache = cache_dir();
        if (!Filesystem::exists(cache))
        {
            Filesystem::create_directories(cache);
        }

        // Assets from the standard resources/assets/ tree are always cached to disk.
        return AssetManager::get().import(source, type, /*cache_compiled=*/true);
    }

} // namespace Ignis
