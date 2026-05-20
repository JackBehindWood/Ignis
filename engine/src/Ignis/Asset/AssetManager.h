#pragma once

#include "Asset.h"
#include "AssetRegistry.h"
#include "AssetLoader.h"
#include "AssetCompiler.h"
#include "Ignis/Foundation/SharedPtr.h"

namespace Ignis
{

    class AssetManager
    {
    public:
        static AssetManager& get()
        {
            static AssetManager instance;
            return instance;
        }

        // Import a raw source file into the registry and return its AssetID.
        // Set cache_compiled = true to allow the compiler to write a .igasset to disk.
        // EditorAssetManager passes true for all assets under resources/assets/.
        AssetID import(const Path& source_path, AssetType type, bool cache_compiled = false);

        // Load (or return cached) asset by ID.
        SharedPtr<Asset> load(AssetID id);

        template<typename T>
        SharedPtr<T> load_as(AssetID id)
        {
            return static_pointer_cast<T>(load(id));
        }

        // Evict from cache and reload from compiled binary.
        void reload(AssetID id);

        // Recompile + reload every registered asset (e.g. bound to a hotkey in the editor).
        void reload_all();

        // Remove registry entries whose source no longer exists, and delete orphaned .igasset files.
        // Call on startup after set_compiled_root() to keep the cache consistent.
        void prune_cache();

        AssetRegistry& registry() { return m_registry; }

        // Set the directory where compiled .igasset binaries are written.
        // Called by EditorAssetManager on startup; defaults to "cache" (relative).
        void set_compiled_root(const Path& dir) { m_compiled_root = dir; }
        const Path& compiled_root() const { return m_compiled_root; }

        // Exposed so asset loaders can trigger on-demand cooking when a .igasset is missing.
        static AssetCompiler* get_compiler(AssetType type);

    private:
        AssetManager() : m_compiled_root("cache") {}

        static AssetLoader* get_loader(AssetType type);

        Path                                     m_compiled_root;
        AssetRegistry                            m_registry;
        UnorderedMap<uint64_t, SharedPtr<Asset>> m_loaded_assets;
    };

} // namespace Ignis
