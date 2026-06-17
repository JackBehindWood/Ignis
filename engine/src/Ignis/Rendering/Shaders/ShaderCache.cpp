#include "igpch.h"
#include "ShaderCache.h"
#include "ShaderCompiler.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/GRI/GRI.h"

namespace Ignis
{

// ---------------------------------------------------------------------------
// IGSH v4 cache blob format — one file per stage
//   ['I','G','S','H'] version=4
//   source_hash_lo u32
//   source_hash_hi u32   (fnv1a-64 of HLSL source content, split lo/hi)
//   target         u8
//   stage_type     u8    entry_point str    bytecode_size u32    bytecode
//   reflection: uniform_buffers storage_buffers storage_textures separate_images separate_samplers
//               stage_inputs stage_outputs push_constants
//               each as count(u32) + entries([name set binding is_unbounded])
//               threadgroup_size_x u32  threadgroup_size_y u32  threadgroup_size_z u32
// ---------------------------------------------------------------------------

static constexpr uint8_t k_magic[4] = {'I', 'G', 'S', 'H'};
static constexpr uint8_t k_version  = 4;

static void write_str(BinaryWriter& w, const String& s)
{
    w.write_u32(static_cast<uint32_t>(s.size()));
    if (!s.empty())
    {
        w.write_bytes(s.data(), s.size());
    }
}

static String read_str(BinaryReader& r)
{
    const uint32_t len = r.read_u32();
    String         s(len, '\0');
    if (len > 0)
    {
        r.read_bytes(s.data(), len);
    }
    return s;
}

static void write_reflection(BinaryWriter& w, const ShaderReflection& refl)
{
    auto write_bindings = [&](const Vector<ShaderResourceBinding>& v)
    {
        w.write_u32(static_cast<uint32_t>(v.size()));
        for (const auto& b : v)
        {
            write_str(w, b.name);
            w.write_u32(b.set);
            w.write_u32(b.binding);
            w.write_u8(b.is_unbounded ? 1u : 0u);
        }
    };
    write_bindings(refl.uniform_buffers);
    write_bindings(refl.storage_buffers);
    write_bindings(refl.storage_textures);
    write_bindings(refl.separate_images);
    write_bindings(refl.separate_samplers);

    auto write_stage_io = [&](const Vector<ShaderStageInput>& v)
    {
        w.write_u32(static_cast<uint32_t>(v.size()));
        for (const auto& i : v)
        {
            write_str(w, i.name);
            w.write_u32(i.location);
        }
    };
    write_stage_io(refl.stage_inputs);
    write_stage_io(refl.stage_outputs);

    w.write_u32(static_cast<uint32_t>(refl.push_constants.size()));
    for (const auto& p : refl.push_constants)
    {
        write_str(w, p.name);
        w.write_u32(p.size);
    }
    w.write_u32(refl.threadgroup_size_x);
    w.write_u32(refl.threadgroup_size_y);
    w.write_u32(refl.threadgroup_size_z);
}

static ShaderReflection read_reflection(BinaryReader& r)
{
    ShaderReflection refl;
    auto             read_bindings = [&](Vector<ShaderResourceBinding>& v)
    {
        const uint32_t n = r.read_u32();
        v.resize(n);
        for (auto& b : v)
        {
            b.name         = read_str(r);
            b.set          = r.read_u32();
            b.binding      = r.read_u32();
            b.is_unbounded = r.read_u8() != 0;
        }
    };
    read_bindings(refl.uniform_buffers);
    read_bindings(refl.storage_buffers);
    read_bindings(refl.storage_textures);
    read_bindings(refl.separate_images);
    read_bindings(refl.separate_samplers);

    auto read_stage_io = [&](Vector<ShaderStageInput>& v)
    {
        const uint32_t n = r.read_u32();
        v.resize(n);
        for (auto& i : v)
        {
            i.name     = read_str(r);
            i.location = r.read_u32();
        }
    };
    read_stage_io(refl.stage_inputs);
    read_stage_io(refl.stage_outputs);

    const uint32_t pc_n = r.read_u32();
    refl.push_constants.resize(pc_n);
    for (auto& p : refl.push_constants)
    {
        p.name = read_str(r);
        p.size = r.read_u32();
    }
    refl.threadgroup_size_x = r.read_u32();
    refl.threadgroup_size_y = r.read_u32();
    refl.threadgroup_size_z = r.read_u32();
    return refl;
}

static void write_stage(BinaryWriter& w, const ShaderStageOutput& s)
{
    w.write_u8(static_cast<uint8_t>(s.stage));
    write_str(w, s.entry_point);
    w.write_u32(static_cast<uint32_t>(s.bytecode.size()));
    w.write_bytes(s.bytecode.data(), s.bytecode.size());
    write_reflection(w, s.reflection);
}

static bool read_stage(BinaryReader& r, ShaderStageOutput& out)
{
    out.stage         = static_cast<GRIShaderStage>(r.read_u8());
    out.entry_point   = read_str(r);
    const uint32_t sz = r.read_u32();
    out.bytecode.resize(sz);
    if (sz > 0)
    {
        r.read_bytes(out.bytecode.data(), sz);
    }
    out.reflection = read_reflection(r);
    return r.good();
}

// ---------------------------------------------------------------------------
// FNV-1a 64-bit — deterministic, no std::hash
// ---------------------------------------------------------------------------

static constexpr uint64_t fnv1a(const char* data, size_t size)
{
    constexpr uint64_t k_basis = 14695981039346656037ULL;
    constexpr uint64_t k_prime = 1099511628211ULL;
    uint64_t           hash    = k_basis;
    for (size_t i = 0; i < size; ++i)
    {
        hash ^= static_cast<uint64_t>(static_cast<uint8_t>(data[i]));
        hash *= k_prime;
    }
    return hash ? hash : 1;
}

static constexpr String hex8(uint64_t v)
{
    constexpr char k_digits[] = "0123456789abcdef";
    String         s(8, '0');
    for (int i = 7; i >= 0; --i)
    {
        s[i] = k_digits[v & 0xF];
        v >>= 4;
    }
    return s;
}

static void ensure_default_stages(ShaderCompilerOptions& opts)
{
    if (opts.count == 0)
    {
        opts.count           = 2;
        opts.stages[0].stage = GRIShaderStage::Vertex;
        opts.stages[1].stage = GRIShaderStage::Pixel;
    }
}

void ShaderCache::set_cache_root(const Path& dir)
{
    m_cache_root = dir;
}

void ShaderCache::set_engine_cache_root(const Path& dir)
{
    m_engine_cache_root = dir;
}

void ShaderCache::set_engine_include_dir(const Path& dir)
{
    m_engine_include_dir = dir;
}

uint64_t ShaderCache::hash_path(const Path& p)
{
    const String s = p.string();
    return fnv1a(s.data(), s.size());
}

uint64_t ShaderCache::make_stage_key(uint64_t path_hash, GRIShaderStage stage)
{
    constexpr uint64_t k_prime = 1099511628211ULL;
    return path_hash ^ (static_cast<uint64_t>(stage) * k_prime);
}

uint64_t ShaderCache::mix_defines(uint64_t base, const Vector<Pair<String, String>>& defines)
{
    constexpr uint64_t k_prime = 1099511628211ULL;
    uint64_t           h       = base;
    for (const auto& [k, v] : defines)
    {
        h ^= fnv1a(k.data(), k.size());
        h *= k_prime;
        h ^= fnv1a(v.data(), v.size());
        h *= k_prime;
    }
    return h ? h : 1;
}

uint64_t ShaderCache::hash_content(const Path& p)
{
    BinaryReader r(p);
    if (!r.is_open())
    {
        return 0;
    }
    const String text = r.read_all_text();
    return fnv1a(text.data(), text.size());
}

Path ShaderCache::cache_file_for(uint64_t variant_key, const Path& source_path, GRIShaderStage stage) const
{
    const String name = source_path.stem().string() + "_" + hex8(make_stage_key(variant_key, stage)) + ".igsh";
    return m_cache_root / name;
}

ShaderTarget ShaderCache::detect_target()
{
    const GRI* gri = RenderSystem::get_gri();
    IG_CORE_ASSERT(gri, "GRI must be initialized before shader compilation");
    switch (gri->get_api())
    {
        case GRIRenderAPI::Metal:
            return ShaderTarget::Metal_MSL;
        default:
            return ShaderTarget::Vulkan_SPIRV;
    }
}

SharedPtr<RenderShader> ShaderCache::make_render_shader(const ShaderStageOutput& s)
{
    GRI* gri = RenderSystem::get_gri();
    if (!gri)
    {
        return nullptr;
    }

    GRIShaderDesc desc;
    desc.stage         = s.stage;
    desc.entry_point   = s.entry_point.c_str();
    desc.bytecode_data = s.bytecode.data();
    desc.bytecode_size = s.bytecode.size();

    if (s.stage == GRIShaderStage::Vertex)
    {
        GRIVertexShaderPtr raw = gri->create_vertex_shader(desc);
        if (!raw)
        {
            IG_CORE_ERROR("ShaderCache: GRI rejected vertex shader (entry='{}')", s.entry_point);
            return nullptr;
        }
        return create_shared<RenderShader>(std::move(raw), GRIShaderStage::Vertex, s.reflection);
    }
    if (s.stage == GRIShaderStage::Pixel)
    {
        GRIPixelShaderPtr raw = gri->create_pixel_shader(desc);
        if (!raw)
        {
            IG_CORE_ERROR("ShaderCache: GRI rejected pixel shader (entry='{}')", s.entry_point);
            return nullptr;
        }
        return create_shared<RenderShader>(std::move(raw), GRIShaderStage::Pixel, s.reflection);
    }
    if (s.stage == GRIShaderStage::Compute)
    {
        GRIComputeShaderPtr raw = gri->create_compute_shader(desc);
        if (!raw)
        {
            IG_CORE_ERROR("ShaderCache: GRI rejected compute shader (entry='{}')", s.entry_point);
            return nullptr;
        }
        return create_shared<RenderShader>(std::move(raw), GRIShaderStage::Compute, s.reflection);
    }

    IG_CORE_ERROR("ShaderCache: unsupported shader stage in make_render_shader");
    return nullptr;
}

// ---------------------------------------------------------------------------

bool ShaderCache::try_load_disk(const Path& cache_file, uint64_t expected_variant_hash, SharedPtr<RenderShader>& out)
{
    BinaryReader r(cache_file);
    if (!r.is_open())
    {
        return false;
    }

    uint8_t file_magic[4];
    r.read_bytes(file_magic, 4);
    if (file_magic[0] != k_magic[0] || file_magic[1] != k_magic[1] || file_magic[2] != k_magic[2] ||
        file_magic[3] != k_magic[3])
    {
        return false;
    }

    const uint8_t ver = r.read_u8();
    if (ver != k_version)
    {
        return false;
    }

    const uint32_t hash_lo     = r.read_u32();
    const uint32_t hash_hi     = r.read_u32();
    const uint64_t stored_hash = (static_cast<uint64_t>(hash_hi) << 32) | hash_lo;
    if (stored_hash != expected_variant_hash)
    {
        return false;
    }

    r.read_u8(); // target

    ShaderStageOutput stage;
    if (!read_stage(r, stage))
    {
        return false;
    }

    out = make_render_shader(stage);
    return out != nullptr;
}

bool ShaderCache::write_disk(const Path& cache_file, uint64_t variant_hash, const ShaderStageOutput& stage,
                             ShaderTarget target)
{
    Filesystem::create_directories(cache_file.parent_path());
    BinaryWriter w(cache_file);
    if (!w.is_open())
    {
        return false;
    }

    w.write_bytes(k_magic, 4);
    w.write_u8(k_version);
    w.write_u32(static_cast<uint32_t>(variant_hash & 0xFFFFFFFFULL));
    w.write_u32(static_cast<uint32_t>((variant_hash >> 32) & 0xFFFFFFFFULL));
    w.write_u8(static_cast<uint8_t>(target));
    write_stage(w, stage);

    return w.good();
}

bool ShaderCache::compile_and_store(const Path& source_path, uint64_t variant_key, uint64_t variant_hash,
                                    GRIShaderStage requested_stage, SharedPtr<RenderShader>& out,
                                    const ShaderCompilerOptions& opts)
{
    BinaryReader src(source_path);
    if (!src.is_open())
    {
        IG_CORE_ERROR("ShaderCache: source not found: {}", source_path.string());
        return false;
    }
    const String source_text = src.read_all_text();

    ShaderCompilerOptions effective_opts = opts;
    ensure_default_stages(effective_opts);

    if (!m_engine_include_dir.empty())
    {
        effective_opts.include_dirs.push_back(m_engine_include_dir);
    }

    const ShaderTarget              target = detect_target();
    ShaderCompiler                  compiler(target);
    const Vector<ShaderStageOutput> stages = compiler.compile(source_text, effective_opts);
    if (stages.empty())
    {
        IG_CORE_ERROR("ShaderCache: compile failed for '{}'", source_path.string());
        return false;
    }

    for (const auto& s : stages)
    {
        const Path cache_file = cache_file_for(variant_key, source_path, s.stage);
        if (!write_disk(cache_file, variant_hash, s, target))
        {
            IG_CORE_WARN("ShaderCache: failed to write cache for '{}'", source_path.string());
        }
        else
        {
            IG_CORE_INFO("ShaderCache: compiled '{}' -> {}", source_path.filename().string(),
                         cache_file.filename().string());
        }

        auto shader = make_render_shader(s);
        if (!shader)
        {
            return false;
        }
        const uint64_t bc_hash = fnv1a(reinterpret_cast<const char*>(s.bytecode.data()), s.bytecode.size());
        {
            UniqueLock<SharedMutex> lock(m_mutex);
            m_memory[make_stage_key(variant_key, s.stage)] = {variant_hash, bc_hash, shader, source_path};
        }
        if (s.stage == requested_stage)
        {
            out = shader;
        }
    }

    return out != nullptr;
}

// ---------------------------------------------------------------------------

void ShaderCache::remove(const Path& source_path)
{
    const uint64_t path_hash   = hash_path(source_path);
    const uint64_t variant_key = mix_defines(path_hash, {});

    GRI* gri = RenderSystem::get_gri();

    for (int s = 0; s < static_cast<int>(GRIShaderStage::COUNT); ++s)
    {
        const auto     stage     = static_cast<GRIShaderStage>(s);
        const uint64_t stage_key = make_stage_key(variant_key, stage);

        {
            UniqueLock<SharedMutex> lock(m_mutex);
            auto                    it = m_memory.find(stage_key);
            if (it != m_memory.end())
            {
                if (gri)
                {
                    gri->invalidate_compiled_shader(it->second.bytecode_hash);
                }
                m_memory.erase(it);
            }
        }

        const Path cache_file = cache_file_for(variant_key, source_path, stage);
        if (Filesystem::exists(cache_file))
        {
            Filesystem::remove(cache_file);
        }
    }
}

SharedPtr<RenderShader> ShaderCache::get_or_compile(const String& source_text, const String& virtual_name,
                                                    GRIShaderStage stage, const ShaderCompilerOptions& opts)
{
    const Path     virtual_path(virtual_name);
    const uint64_t path_hash    = hash_path(virtual_path);
    const uint64_t content_hash = fnv1a(source_text.data(), source_text.size());
    const uint64_t variant_hash = mix_defines(content_hash, opts.defines);
    const uint64_t variant_key  = mix_defines(path_hash, opts.defines);
    const uint64_t stage_key    = make_stage_key(variant_key, stage);

    {
        SharedLock<SharedMutex> lock(m_mutex);
        auto                    it = m_memory.find(stage_key);
        if (it != m_memory.end() && it->second.variant_hash == variant_hash)
        {
            SharedPtr<RenderShader> sh = it->second.shader;
            if (sh)
            {
                return sh;
            }
        }
    }

    const Path              cache_file = cache_file_for(variant_key, virtual_path, stage);
    SharedPtr<RenderShader> sh;
    if (try_load_disk(cache_file, variant_hash, sh) ||
        (m_engine_cache_root != m_cache_root &&
         try_load_disk(m_engine_cache_root / cache_file.filename(), variant_hash, sh)))
    {
        UniqueLock<SharedMutex> lock(m_mutex);
        m_memory[stage_key] = {variant_hash, 0u, sh, virtual_path};
        return sh;
    }

    if (!compile_text_and_store(source_text, virtual_path, variant_key, variant_hash, stage, sh, opts))
    {
        return nullptr;
    }

    return sh;
}

SharedPtr<RenderShader> ShaderCache::get_or_compile(const Path& source_path, GRIShaderStage stage,
                                                    const ShaderCompilerOptions& opts)
{
    const uint64_t path_hash    = hash_path(source_path);
    const uint64_t content_hash = hash_content(source_path);
    const uint64_t variant_hash = mix_defines(content_hash, opts.defines);
    const uint64_t variant_key  = mix_defines(path_hash, opts.defines);
    const uint64_t stage_key    = make_stage_key(variant_key, stage);

    {
        SharedLock<SharedMutex> lock(m_mutex);
        auto                    it = m_memory.find(stage_key);
        if (it != m_memory.end() && it->second.variant_hash == variant_hash)
        {
            SharedPtr<RenderShader> sh = it->second.shader;
            if (sh)
            {
                return sh;
            }
        }
    }

    const Path              cache_file = cache_file_for(variant_key, source_path, stage);
    SharedPtr<RenderShader> sh;
    if (try_load_disk(cache_file, variant_hash, sh) ||
        (m_engine_cache_root != m_cache_root &&
         try_load_disk(m_engine_cache_root / cache_file.filename(), variant_hash, sh)))
    {
        UniqueLock<SharedMutex> lock(m_mutex);
        m_memory[stage_key] = {variant_hash, 0u, sh, source_path};
        return sh;
    }

    if (!compile_and_store(source_path, variant_key, variant_hash, stage, sh, opts))
    {
        return nullptr;
    }

    return sh;
}

// ---------------------------------------------------------------------------

bool ShaderCache::compile_text_and_store(const String& source_text, const Path& virtual_path, uint64_t variant_key,
                                         uint64_t variant_hash, GRIShaderStage requested_stage,
                                         SharedPtr<RenderShader>& out, const ShaderCompilerOptions& opts)
{
    ShaderCompilerOptions effective_opts = opts;
    ensure_default_stages(effective_opts);

    if (!m_engine_include_dir.empty())
    {
        effective_opts.include_dirs.push_back(m_engine_include_dir);
    }

    const ShaderTarget              target = detect_target();
    ShaderCompiler                  compiler(target);
    const Vector<ShaderStageOutput> stages = compiler.compile(source_text, effective_opts);
    if (stages.empty())
    {
        IG_CORE_ERROR("ShaderCache: compile failed for inline shader '{}'", virtual_path.string());
        return false;
    }

    for (const auto& s : stages)
    {
        const Path cf = cache_file_for(variant_key, virtual_path, s.stage);
        if (!write_disk(cf, variant_hash, s, target))
        {
            IG_CORE_WARN("ShaderCache: failed to write cache for inline shader '{}'", virtual_path.string());
        }
        else
        {
            IG_CORE_INFO("ShaderCache: compiled inline '{}' -> {}", virtual_path.string(), cf.filename().string());
        }

        auto shader = make_render_shader(s);
        if (!shader)
        {
            return false;
        }
        const uint64_t bc_hash = fnv1a(reinterpret_cast<const char*>(s.bytecode.data()), s.bytecode.size());
        {
            UniqueLock<SharedMutex> lock(m_mutex);
            m_memory[make_stage_key(variant_key, s.stage)] = {variant_hash, bc_hash, shader, virtual_path};
        }
        if (s.stage == requested_stage)
        {
            out = shader;
        }
    }

    return out != nullptr;
}

// ---------------------------------------------------------------------------

SharedPtr<RenderShader> ShaderCache::get_or_compile_async(const Path& source_path, GRIShaderStage stage,
                                                          const ShaderCompilerOptions& opts)
{
    const uint64_t path_hash    = hash_path(source_path);
    const uint64_t content_hash = hash_content(source_path);
    const uint64_t variant_hash = mix_defines(content_hash, opts.defines);
    const uint64_t variant_key  = mix_defines(path_hash, opts.defines);
    const uint64_t stage_key    = make_stage_key(variant_key, stage);

    {
        SharedLock<SharedMutex> lock(m_mutex);
        auto                    it = m_memory.find(stage_key);
        if (it != m_memory.end() && it->second.variant_hash == variant_hash && it->second.shader)
        {
            return it->second.shader;
        }
    }

    {
        UniqueLock<Mutex> plock(m_pending_mutex);
        auto              pit = m_pending.find(variant_key);
        if (pit != m_pending.end())
        {
            if (pit->second.wait_for(std::chrono::nanoseconds(0)) == FutureStatus::ready)
            {
                pit->second.get();
                m_pending.erase(pit);
                plock.unlock();
                SharedLock<SharedMutex> lock(m_mutex);
                auto                    it = m_memory.find(stage_key);
                if (it != m_memory.end() && it->second.shader)
                {
                    return it->second.shader;
                }
            }
            return nullptr;
        }

        {
            SharedLock<SharedMutex> lock(m_mutex);
            auto                    it = m_memory.find(stage_key);
            if (it != m_memory.end() && it->second.variant_hash == variant_hash && it->second.shader)
            {
                return it->second.shader;
            }
        }

        m_pending.emplace(variant_key,
                          std::async(std::launch::async,
                                     [this, source_path, variant_key, variant_hash, stage, opts]() -> bool
                                     {
                                         try
                                         {
                                             SharedPtr<RenderShader> dummy;
                                             return compile_and_store(source_path, variant_key, variant_hash, stage,
                                                                      dummy, opts);
                                         }
                                         catch (...)
                                         {
                                             IG_CORE_ERROR("ShaderCache: async compile threw unexpectedly for '{}'",
                                                           source_path.string());
                                             return false;
                                         }
                                     }));
    }

    return nullptr;
}

SharedPtr<RenderShader> ShaderCache::get_or_compile_async(const String& source_text, const String& virtual_name,
                                                          GRIShaderStage stage, const ShaderCompilerOptions& opts)
{
    const Path     virtual_path(virtual_name);
    const uint64_t path_hash    = hash_path(virtual_path);
    const uint64_t content_hash = fnv1a(source_text.data(), source_text.size());
    const uint64_t variant_hash = mix_defines(content_hash, opts.defines);
    const uint64_t variant_key  = mix_defines(path_hash, opts.defines);
    const uint64_t stage_key    = make_stage_key(variant_key, stage);

    {
        SharedLock<SharedMutex> lock(m_mutex);
        auto                    it = m_memory.find(stage_key);
        if (it != m_memory.end() && it->second.variant_hash == variant_hash && it->second.shader)
        {
            return it->second.shader;
        }
    }

    {
        UniqueLock<Mutex> plock(m_pending_mutex);
        auto              pit = m_pending.find(variant_key);
        if (pit != m_pending.end())
        {
            if (pit->second.wait_for(std::chrono::nanoseconds(0)) == FutureStatus::ready)
            {
                pit->second.get();
                m_pending.erase(pit);
                plock.unlock();
                SharedLock<SharedMutex> lock(m_mutex);
                auto                    it = m_memory.find(stage_key);
                if (it != m_memory.end() && it->second.shader)
                {
                    return it->second.shader;
                }
            }
            return nullptr;
        }

        {
            SharedLock<SharedMutex> lock(m_mutex);
            auto                    it = m_memory.find(stage_key);
            if (it != m_memory.end() && it->second.variant_hash == variant_hash && it->second.shader)
            {
                return it->second.shader;
            }
        }

        m_pending.emplace(variant_key,
                          std::async(std::launch::async,
                                     [this, source_text, virtual_path, variant_key, variant_hash, stage, opts]() -> bool
                                     {
                                         try
                                         {
                                             SharedPtr<RenderShader> dummy;
                                             return compile_text_and_store(source_text, virtual_path, variant_key,
                                                                           variant_hash, stage, dummy, opts);
                                         }
                                         catch (...)
                                         {
                                             IG_CORE_ERROR("ShaderCache: async compile threw unexpectedly for '{}'",
                                                           virtual_path.string());
                                             return false;
                                         }
                                     }));
    }

    return nullptr;
}

// ---------------------------------------------------------------------------

Vector<ShaderCache::ShaderCacheStat> ShaderCache::snapshot_stats() const
{
    SharedLock<SharedMutex> lock(m_mutex);
    Vector<ShaderCacheStat> out;
    out.reserve(m_memory.size());
    for (const auto& [key, entry] : m_memory)
    {
        out.push_back({entry.source_path, entry.variant_hash, entry.bytecode_hash});
    }
    return out;
}

} // namespace Ignis
