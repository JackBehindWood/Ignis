#include "igpch.h"
#include "AssetManager.h"

namespace Ignis
{

    AssetLoader* AssetManager::get_loader(AssetType type)
    {
        switch (type)
        {
            // case AssetType::Texture2D: { static Texture2DLoader s; return &s; }
            // case AssetType::Shader:    { static ShaderLoader    s; return &s; }
            default: return nullptr;
        }
    }

    AssetCompiler* AssetManager::get_compiler(AssetType type)
    {
        switch (type)
        {
            // case AssetType::Texture2D: { static Texture2DCompiler s; return &s; }
            // case AssetType::Shader:    { static ShaderCompiler    s; return &s; }
            default: return nullptr;
        }
    }

    AssetID AssetManager::import(const Path& source_path, AssetType type)
    {
        AssetID existing = m_registry.find_by_source(source_path);
        if (static_cast<uint64_t>(existing) != UUID::s_invalid)
            return existing;

        AssetMetadata metadata;
        metadata.ID            = AssetID{};
        metadata.Type          = type;
        metadata.source_path   = source_path;
        metadata.compiled_path = m_compiled_root / (std::to_string(static_cast<uint64_t>(metadata.ID)) + ".igasset");
        m_registry.register_asset(metadata);
        return metadata.ID;
    }

    SharedPtr<Asset> AssetManager::load(AssetID id)
    {
        auto it = m_loaded_assets.find(static_cast<uint64_t>(id));
        if (it != m_loaded_assets.end())
            return it->second;

        const AssetMetadata* metadata = m_registry.get(id);
        if (!metadata || !metadata->is_valid())
            return nullptr;

        AssetLoader* loader = get_loader(metadata->Type);
        if (!loader)
            return nullptr;

        SharedPtr<Asset> asset = loader->load(*metadata);
        if (asset)
            m_loaded_assets[static_cast<uint64_t>(id)] = asset;

        return asset;
    }

    void AssetManager::reload(AssetID id)
    {
        const AssetMetadata* metadata = m_registry.get(id);
        if (!metadata || !metadata->is_valid())
            return;

        AssetCompiler* compiler = get_compiler(metadata->Type);
        if (compiler)
            compiler->compile(*metadata);

        m_loaded_assets.erase(static_cast<uint64_t>(id));
    }

    void AssetManager::reload_all()
    {
        for (const auto& [key, metadata] : m_registry.get_all())
        {
            AssetCompiler* compiler = get_compiler(metadata.Type);
            if (compiler)
                compiler->compile(metadata);
        }
        m_loaded_assets.clear();
    }

} // namespace Ignis
