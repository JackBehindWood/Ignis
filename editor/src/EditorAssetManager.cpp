#include "EditorAssetManager.h"
#include "Ignis/Asset/AssetManager.h"

namespace Ignis
{

    static Path subdir_for(AssetType type)
    {
        switch (type)
        {
            case AssetType::Texture2D: return "textures";
            case AssetType::Shader:    return "shaders";
            case AssetType::Mesh:      return "meshes";
            case AssetType::Material:  return "materials";
            default:                   return "";
        }
    }

    EditorAssetManager::EditorAssetManager()
    {
        set_root(Filesystem::current_path() / "resources");
    }

    void EditorAssetManager::set_root(const Path& root)
    {
        m_root = root;
    }

    AssetID EditorAssetManager::import_texture(const Path& filename)
    {
        return import(Path("assets/textures") / filename, AssetType::Texture2D);
    }

    std::pair<AssetID, AssetID> EditorAssetManager::import_shader(const Path& filename)
    {
        Path source = m_root / "assets/shaders" / filename;

        if (!Filesystem::exists(source))
        {
            IG_ERROR("EditorAssetManager: source file not found: {0}", source.string());
            const AssetID invalid(UUID::s_invalid);
            return { invalid, invalid };
        }

        const AssetID vs_id = AssetManager::get().import(
            source, AssetType::Shader, true, (uint64_t)GRIShaderStage::Vertex);
        const AssetID ps_id = AssetManager::get().import(
            source, AssetType::Shader, true, (uint64_t)GRIShaderStage::Pixel);

        return { vs_id, ps_id };
    }

    AssetID EditorAssetManager::import_mesh(const Path& filename)
    {
        return import(Path("assets/meshes") / filename, AssetType::Mesh);
    }

    AssetID EditorAssetManager::import_material(const Path& filename)
    {
        return import(Path("assets/materials") / filename, AssetType::Material);
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

    AssetID EditorAssetManager::import(const Path& relative_path, AssetType type)
    {
        Path source = m_root / relative_path;

        if (!Filesystem::exists(source))
        {
            IG_ERROR("EditorAssetManager: source file not found: {0}", source.string());
            return AssetID(UUID::s_invalid);
        }

        return AssetManager::get().import(source, type, /*cache_compiled=*/true);
    }

} // namespace Ignis
