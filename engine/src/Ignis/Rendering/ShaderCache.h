#pragma once

#include "ShaderTarget.h"
#include "RenderShader.h"

namespace Ignis
{
    struct ShaderStageOutput;

    // Compiled-bytecode cache, decoupled from the Asset system.
    // Keyed by source-file content hash — a changed HLSL automatically triggers recompile.
    // Call set_cache_root() once at startup (or on project load) before first use.
    // Internal renderer shaders bypass AssetManager and call get_or_compile() directly.
    class ShaderCache
    {
    public:
        static ShaderCache& get();

        void        set_cache_root(const Path& dir);
        const Path& cache_root() const { return m_cache_root; }

        SharedPtr<RenderShader> get_or_compile(const Path& source_path);
        SharedPtr<RenderShader> get_or_compile(const String& source_text, const String& virtual_name);

        void remove(const Path& source_path);
    private:
        ShaderCache() : m_cache_root("shadercache") {}

        struct CacheEntry
        {
            uint64_t             source_hash;
            SharedPtr<RenderShader> shader;
        };

        Path                                   m_cache_root;
        UnorderedMap<uint64_t, CacheEntry>     m_memory;   // key: path_hash(source_path)

        Path     cache_file_for(uint64_t path_hash, const Path& source_path) const;

        static uint64_t hash_path(const Path& p);
        static uint64_t hash_content(const Path& p);
        static ShaderTarget detect_target();
        static SharedPtr<RenderShader> make_render_shader(const Vector<ShaderStageOutput>& stages);

        SharedPtr<RenderShader> try_load_disk(const Path& cache_file, uint64_t expected_content_hash);
        bool                    write_disk(const Path& cache_file, uint64_t content_hash,
                                           const Vector<ShaderStageOutput>& stages, ShaderTarget target);
        SharedPtr<RenderShader> compile_and_store(const Path& source_path,
                                                   uint64_t path_hash, uint64_t content_hash);
    };

} // namespace Ignis
