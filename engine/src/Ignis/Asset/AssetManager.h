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
        AssetID import(const Path& source_path, AssetType type);

        // Load (or return cached) asset by ID.
        SharedPtr<Asset> load(AssetID id);

        template<typename T>
        SharedPtr<T> load_as(AssetID id)
        {
            return SharedPtr<T>(load(id));
        }

        // Evict from cache and reload from compiled binary.
        void reload(AssetID id);

        // Recompile + reload every registered asset (e.g. bound to a hotkey in the editor).
        void reload_all();

        AssetRegistry& registry() { return m_registry; }

        // Set the directory where compiled .igasset binaries are written.
        // Called by EditorAssetManager on startup; defaults to "cache" (relative).
        void set_compiled_root(const Path& dir) { m_compiled_root = dir; }
        const Path& compiled_root() const { return m_compiled_root; }

    private:
        AssetManager() : m_compiled_root("cache") {}

        static AssetLoader*   get_loader(AssetType type);
        static AssetCompiler* get_compiler(AssetType type);

        Path                                     m_compiled_root;
        AssetRegistry                            m_registry;
        UnorderedMap<uint64_t, SharedPtr<Asset>> m_loaded_assets;
    };

} // namespace Ignis
