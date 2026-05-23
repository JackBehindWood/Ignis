#include "igpch.h"
#include "AssetManager.h"

#include "Ignis/Rendering/ShaderCache.h"
#include "Ignis/Asset/AssetShaderCompiler.h"
#include "Ignis/Asset/AssetMeshCompiler.h"
#include "Ignis/Asset/AssetMaterialCompiler.h"
#include "Ignis/Asset/AssetTexture2DCompiler.h"
#include "Ignis/Asset/ShaderLoader.h"
#include "Ignis/Asset/MeshLoader.h"
#include "Ignis/Asset/MaterialLoader.h"
#include "Ignis/Asset/TextureLoader.h"

namespace Ignis
{

    AssetLoader* AssetManager::get_loader(AssetType type)
    {
        switch (type)
        {
            case AssetType::Texture2D: { static TextureLoader          s; return &s; }
            case AssetType::Shader:   { static ShaderLoader   s; return &s; }
            case AssetType::Mesh:     { static MeshLoader     s; return &s; }
            case AssetType::Material: { static MaterialLoader s; return &s; }
            default: return nullptr;
        }
    }

    AssetCompiler* AssetManager::get_compiler(AssetType type)
    {
        switch (type)
        {
            case AssetType::Texture2D: { static AssetTexture2DCompiler s; return &s; }
            case AssetType::Shader:   { static AssetShaderCompiler   s; return &s; }
            case AssetType::Mesh:     { static AssetMeshCompiler     s; return &s; }
            case AssetType::Material: { static AssetMaterialCompiler s; return &s; }
            default: return nullptr;
        }
    }

    // FNV-1a 64-bit hash — deterministic across runs, unlike std::hash.
    static uint64_t hash_path(const Path& path)
    {
        constexpr uint64_t k_basis = 14695981039346656037ULL;
        constexpr uint64_t k_prime = 1099511628211ULL;

        const String str = path.string();
        uint64_t hash = k_basis;
        for (char c : str)
        {
            hash ^= static_cast<uint64_t>(static_cast<uint8_t>(c));
            hash *= k_prime;
        }
        return hash ? hash : 1; // 0 is UUID::s_invalid, so nudge it
    }

    // e.g. hash_path(...) → "a3f2b1c0" (first 8 hex digits, enough to avoid collisions)
    static String hash_hex8(uint64_t hash)
    {
        constexpr char k_digits[] = "0123456789abcdef";
        String result(8, '0');
        for (int i = 7; i >= 0; --i)
        {
            result[i] = k_digits[hash & 0xF];
            hash >>= 4;
        }
        return result;
    }

    AssetID AssetManager::import(const Path& source_path, AssetType type, bool cache_compiled, uint64_t user_data)
    {
        // When user_data is non-zero, incorporate it into the key so that two sub-assets
        // of the same source (e.g. VS and PS from the same .hlsl) get distinct IDs.
        const String key_str = user_data
            ? source_path.string() + "|" + std::to_string(user_data)
            : source_path.string();
        const Path key_path(key_str);

        AssetID existing = m_registry.find_by_source(key_path);
        if (static_cast<uint64_t>(existing) != UUID::s_invalid)
            return existing;

        const uint64_t hash = hash_path(key_path);

        // e.g. triangle_a3f2b1c0.igasset — readable and collision-safe
        const String cache_name = source_path.stem().string() + "_" + hash_hex8(hash) + ".igasset";

        AssetMetadata metadata;
        metadata.ID             = AssetID(hash);
        metadata.Type           = type;
        metadata.source_path    = source_path;
        metadata.compiled_path  = m_compiled_root / cache_name;
        metadata.cache_compiled = cache_compiled;
        metadata.user_data      = user_data;
        m_registry.register_asset(metadata, key_path);
        return metadata.ID;
    }

    void AssetManager::prune_cache()
    {
        // Remove entries whose source file has been deleted or moved.
        Vector<AssetID> dead;
        for (const auto& [id, metadata] : m_registry.get_all())
        {
            if (!Filesystem::exists(metadata.source_path))
                dead.push_back(metadata.ID);
        }
        for (AssetID id : dead)
        {
            if (const AssetMetadata* meta = m_registry.get(id))
            {
                if (meta->Type == AssetType::Shader)
                    ShaderCache::get().remove(meta->source_path);
                Filesystem::remove(meta->compiled_path);
            }
            m_registry.remove(id);
            m_loaded_assets.erase(static_cast<uint64_t>(id));
        }

        // Delete any .igasset files in the cache dir that no registry entry points to.
        if (!Filesystem::exists(m_compiled_root))
            return;

        std::unordered_set<String> known;
        for (const auto& [id, metadata] : m_registry.get_all())
            known.insert(metadata.compiled_path.string());

        for (const auto& entry : Filesystem::directory_iterator(m_compiled_root))
        {
            if (entry.path().extension() == ".igasset" && !known.count(entry.path().string()))
                Filesystem::remove(entry.path());
        }
    }

    SharedPtr<Asset> AssetManager::load(AssetID id)
    {
        auto it = m_loaded_assets.find(static_cast<uint64_t>(id));
        if (it != m_loaded_assets.end())
        {
            return it->second;
        }

        const AssetMetadata* metadata = m_registry.get(id);
        if (!metadata || !metadata->is_valid())
        {
            return nullptr;
        }

        AssetLoader* loader = get_loader(metadata->Type);
        if (!loader)
        {
            return nullptr;
        }

        SharedPtr<Asset> asset = loader->load(*metadata);
        if (asset)
        {
            m_loaded_assets[static_cast<uint64_t>(id)] = asset;
        }

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
            if (metadata.Type == AssetType::Shader)
                ShaderCache::get().remove(metadata.source_path);

            AssetCompiler* compiler = get_compiler(metadata.Type);
            if (compiler)
                compiler->compile(metadata);
        }
        m_loaded_assets.clear();
    }

    AssetID AssetManager::create_mesh(const Vector<uint8_t>& vertices, const Vector<uint32_t>& indices, uint32_t vertex_stride)
    {
        AssetID id;
        m_loaded_assets[static_cast<uint64_t>(id)] = create_shared<AssetMesh>(id, vertices, indices, vertex_stride);
        return id;
    }

} // namespace Ignis
