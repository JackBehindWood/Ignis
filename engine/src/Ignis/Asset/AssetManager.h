#pragma once

#include "Asset.h"
#include "AssetMesh.h"
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

        AssetID import(const Path& source_path, AssetType type, bool cache_compiled = false);

        SharedPtr<Asset> load(AssetID id);

        template<typename T>
        SharedPtr<T> load_as(AssetID id)
        {
            return static_pointer_cast<T>(load(id));
        }

        void reload(AssetID id);

        void reload_all();

        void prune_cache();

        AssetRegistry& registry() { return m_registry; }

        AssetID create_mesh(const Vector<uint8_t>& vertices, const Vector<uint32_t>& indices);

        void set_compiled_root(const Path& dir) { m_compiled_root = dir; }
        const Path& compiled_root() const { return m_compiled_root; }

        static AssetCompiler* get_compiler(AssetType type);

    private:
        AssetManager() : m_compiled_root("cache") {}

        static AssetLoader* get_loader(AssetType type);

        Path                                     m_compiled_root;
        AssetRegistry                            m_registry;
        UnorderedMap<uint64_t, SharedPtr<Asset>> m_loaded_assets;
    };

} // namespace Ignis
