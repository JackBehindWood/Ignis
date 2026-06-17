#pragma once

#include "ShaderTarget.h"
#include "RenderShader.h"
#include "ShaderCompiler.h"

namespace Ignis
{
struct ShaderStageOutput;

class ShaderCache
{
public:
    static ShaderCache& get()
    {
        static ShaderCache instance;
        return instance;
    }

    void        set_cache_root(const Path& dir);
    void        set_engine_cache_root(const Path& dir);
    void        set_engine_include_dir(const Path& dir);
    const Path& cache_root() const
    {
        return m_cache_root;
    }

    SharedPtr<RenderShader> get_or_compile(const Path& source_path, GRIShaderStage stage,
                                           const ShaderCompilerOptions& opts = {});
    SharedPtr<RenderShader> get_or_compile(const String& source_text, const String& virtual_name, GRIShaderStage stage,
                                           const ShaderCompilerOptions& opts = {});

    SharedPtr<RenderShader> get_or_compile_async(const Path& source_path, GRIShaderStage stage,
                                                 const ShaderCompilerOptions& opts = {});
    SharedPtr<RenderShader> get_or_compile_async(const String& source_text, const String& virtual_name,
                                                 GRIShaderStage stage, const ShaderCompilerOptions& opts = {});

    void remove(const Path& source_path);

    struct ShaderCacheStat
    {
        Path     source_path;
        uint64_t variant_hash;
        uint64_t bytecode_hash;
    };
    Vector<ShaderCacheStat> snapshot_stats() const;

private:
    ShaderCache()
        : m_cache_root("shadercache"),
          m_engine_cache_root("shadercache")
    {
    }

    struct CacheEntry
    {
        uint64_t variant_hash;
        uint64_t bytecode_hash; // fnv1a of the backend bytecode; used to evict MetalShaderLibrary entries
        SharedPtr<RenderShader> shader;
        Path                    source_path;
    };

    Path                                 m_cache_root;
    Path                                 m_engine_cache_root;
    Path                                 m_engine_include_dir;
    mutable SharedMutex                  m_mutex;
    UnorderedMap<uint64_t, CacheEntry>   m_memory;
    mutable Mutex                        m_pending_mutex;
    UnorderedMap<uint64_t, Future<bool>> m_pending;

    Path cache_file_for(uint64_t variant_key, const Path& source_path, GRIShaderStage stage) const;

    static uint64_t                hash_path(const Path& p);
    static uint64_t                hash_content(const Path& p);
    static uint64_t                make_stage_key(uint64_t variant_key, GRIShaderStage stage);
    static uint64_t                mix_defines(uint64_t base, const Vector<Pair<String, String>>& defines);
    static ShaderTarget            detect_target();
    static SharedPtr<RenderShader> make_render_shader(const ShaderStageOutput& stage);
    bool try_load_disk(const Path& cache_file, uint64_t expected_variant_hash, SharedPtr<RenderShader>& out);
    bool write_disk(const Path& cache_file, uint64_t variant_hash, const ShaderStageOutput& stage, ShaderTarget target);
    bool compile_and_store(const Path& source_path, uint64_t variant_key, uint64_t variant_hash,
                           GRIShaderStage requested_stage, SharedPtr<RenderShader>& out,
                           const ShaderCompilerOptions& opts);
    bool compile_text_and_store(const String& source_text, const Path& virtual_path, uint64_t variant_key,
                                uint64_t variant_hash, GRIShaderStage requested_stage, SharedPtr<RenderShader>& out,
                                const ShaderCompilerOptions& opts);
};

} // namespace Ignis
