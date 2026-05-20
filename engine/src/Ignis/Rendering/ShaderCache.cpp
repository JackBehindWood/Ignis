#include "igpch.h"
#include "ShaderCache.h"
#include "ShaderCompiler.h"
#include "RenderSystem.h"

namespace Ignis
{

// ---------------------------------------------------------------------------
// IGSH v8 cache blob format
//   ['I','G','S','H'] version=8
//   source_hash_lo u32
//   source_hash_hi u32
//   target         u8
//   num_stages     u8
//   per stage: stage_type(u8) entry_point(str) bytecode_size(u32) bytecode reflection
// ---------------------------------------------------------------------------

static constexpr uint8_t  k_magic[4]  = {'I', 'G', 'S', 'H'};
static constexpr uint8_t  k_version   = 8;

static void write_str(BinaryWriter& w, const String& s)
{
    w.write_u32(static_cast<uint32_t>(s.size()));
    if (!s.empty())
        w.write_bytes(s.data(), s.size());
}

static String read_str(BinaryReader& r)
{
    const uint32_t len = r.read_u32();
    String s(len, '\0');
    if (len > 0)
        r.read_bytes(s.data(), len);
    return s;
}

static void write_reflection(BinaryWriter& w, const ShaderReflection& refl)
{
    auto write_bindings = [&](const Vector<ShaderResourceBinding>& v)
    {
        w.write_u32(static_cast<uint32_t>(v.size()));
        for (const auto& b : v) { write_str(w, b.name); w.write_u32(b.set); w.write_u32(b.binding); }
    };
    write_bindings(refl.uniform_buffers);
    write_bindings(refl.storage_buffers);
    write_bindings(refl.separate_images);
    write_bindings(refl.separate_samplers);

    w.write_u32(static_cast<uint32_t>(refl.stage_inputs.size()));
    for (const auto& i : refl.stage_inputs) { write_str(w, i.name); w.write_u32(i.location); }

    w.write_u32(static_cast<uint32_t>(refl.push_constants.size()));
    for (const auto& p : refl.push_constants) { write_str(w, p.name); w.write_u32(p.size); }
}

static ShaderReflection read_reflection(BinaryReader& r)
{
    ShaderReflection refl;
    auto read_bindings = [&](Vector<ShaderResourceBinding>& v)
    {
        const uint32_t n = r.read_u32();
        v.resize(n);
        for (auto& b : v) { b.name = read_str(r); b.set = r.read_u32(); b.binding = r.read_u32(); }
    };
    read_bindings(refl.uniform_buffers);
    read_bindings(refl.storage_buffers);
    read_bindings(refl.separate_images);
    read_bindings(refl.separate_samplers);

    const uint32_t si_n = r.read_u32();
    refl.stage_inputs.resize(si_n);
    for (auto& i : refl.stage_inputs) { i.name = read_str(r); i.location = r.read_u32(); }

    const uint32_t pc_n = r.read_u32();
    refl.push_constants.resize(pc_n);
    for (auto& p : refl.push_constants) { p.name = read_str(r); p.size = r.read_u32(); }
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
    out.stage       = static_cast<GRIShaderStage>(r.read_u8());
    out.entry_point = read_str(r);
    const uint32_t sz = r.read_u32();
    out.bytecode.resize(sz);
    if (sz > 0)
        r.read_bytes(out.bytecode.data(), sz);
    out.reflection = read_reflection(r);
    return r.good();
}

// ---------------------------------------------------------------------------
// FNV-1a 64-bit — deterministic, no std::hash
// ---------------------------------------------------------------------------

static uint64_t fnv1a(const char* data, size_t size)
{
    constexpr uint64_t k_basis = 14695981039346656037ULL;
    constexpr uint64_t k_prime = 1099511628211ULL;
    uint64_t hash = k_basis;
    for (size_t i = 0; i < size; ++i)
    {
        hash ^= static_cast<uint64_t>(static_cast<uint8_t>(data[i]));
        hash *= k_prime;
    }
    return hash ? hash : 1;
}

static String hex8(uint64_t v)
{
    constexpr char k_digits[] = "0123456789abcdef";
    String s(8, '0');
    for (int i = 7; i >= 0; --i) { s[i] = k_digits[v & 0xF]; v >>= 4; }
    return s;
}

// ---------------------------------------------------------------------------
// ShaderCache
// ---------------------------------------------------------------------------

ShaderCache& ShaderCache::get()
{
    static ShaderCache s_instance;
    return s_instance;
}

void ShaderCache::set_cache_root(const Path& dir)
{
    m_cache_root = dir;
}

// ---------------------------------------------------------------------------

uint64_t ShaderCache::hash_path(const Path& p)
{
    const String s = p.string();
    return fnv1a(s.data(), s.size());
}

uint64_t ShaderCache::hash_content(const Path& p)
{
    BinaryReader r(p);
    if (!r.is_open())
        return 0;
    const String text = r.read_all_text();
    return fnv1a(text.data(), text.size());
}

Path ShaderCache::cache_file_for(uint64_t path_hash, const Path& source_path) const
{
    const String name = source_path.stem().string() + "_" + hex8(path_hash) + ".igsh";
    return m_cache_root / name;
}

ShaderTarget ShaderCache::detect_target()
{
    const GRI* gri = RenderSystem::get_gri();
    IG_CORE_ASSERT(gri, "GRI must be initialized before shader compilation");
    switch (gri->get_api())
    {
        case GRIRenderAPI::Metal: return ShaderTarget::Metal_MSL;
        default:                  return ShaderTarget::Vulkan_SPIRV;
    }
}

SharedPtr<RenderShader> ShaderCache::make_render_shader(const Vector<ShaderStageOutput>& stages)
{
    GRI* gri = RenderSystem::get_gri();
    if (!gri)
        return nullptr;

    GRIVertexShaderPtr vs;
    GRIPixelShaderPtr  ps;
    ShaderReflection   vs_refl, ps_refl;

    for (const auto& s : stages)
    {
        GRIShaderDesc desc;
        desc.stage         = s.stage;
        desc.entry_point   = s.entry_point.c_str();
        desc.bytecode_data = s.bytecode.data();
        desc.bytecode_size = s.bytecode.size();

        if (s.stage == GRIShaderStage::Vertex)
        {
            vs      = gri->create_vertex_shader(desc);
            vs_refl = s.reflection;
        }
        else if (s.stage == GRIShaderStage::Pixel)
        {
            ps      = gri->create_pixel_shader(desc);
            ps_refl = s.reflection;
        }
    }

    if (!vs || !ps)
        return nullptr;

    return create_shared<RenderShader>(std::move(vs), std::move(ps),
                                       std::move(vs_refl), std::move(ps_refl));
}

// ---------------------------------------------------------------------------

SharedPtr<RenderShader> ShaderCache::try_load_disk(const Path& cache_file, uint64_t expected_content_hash)
{
    BinaryReader r(cache_file);
    if (!r.is_open())
        return nullptr;

    uint8_t file_magic[4];
    r.read_bytes(file_magic, 4);
    if (file_magic[0] != k_magic[0] || file_magic[1] != k_magic[1] ||
        file_magic[2] != k_magic[2] || file_magic[3] != k_magic[3])
        return nullptr;

    const uint8_t ver = r.read_u8();
    if (ver != k_version)
        return nullptr;

    const uint32_t hash_lo = r.read_u32();
    const uint32_t hash_hi = r.read_u32();
    const uint64_t stored_hash = (static_cast<uint64_t>(hash_hi) << 32) | hash_lo;
    if (stored_hash != expected_content_hash)
        return nullptr; // stale

    r.read_u8(); // target — already chosen by detect_target() at runtime
    const uint8_t num_stages = r.read_u8();

    Vector<ShaderStageOutput> stages(num_stages);
    for (uint8_t i = 0; i < num_stages; ++i)
    {
        if (!read_stage(r, stages[i]))
            return nullptr;
    }

    return make_render_shader(stages);
}

bool ShaderCache::write_disk(const Path& cache_file, uint64_t content_hash,
                              const Vector<ShaderStageOutput>& stages, ShaderTarget target)
{
    Filesystem::create_directories(cache_file.parent_path());
    BinaryWriter w(cache_file);
    if (!w.is_open())
        return false;

    w.write_bytes(k_magic, 4);
    w.write_u8(k_version);
    w.write_u32(static_cast<uint32_t>(content_hash & 0xFFFFFFFFULL));
    w.write_u32(static_cast<uint32_t>((content_hash >> 32) & 0xFFFFFFFFULL));
    w.write_u8(static_cast<uint8_t>(target));
    w.write_u8(static_cast<uint8_t>(stages.size()));
    for (const auto& s : stages)
        write_stage(w, s);

    return w.good();
}

SharedPtr<RenderShader> ShaderCache::compile_and_store(const Path& source_path,
                                                        uint64_t path_hash, uint64_t content_hash)
{
    BinaryReader src(source_path);
    if (!src.is_open())
    {
        IG_CORE_ERROR("ShaderCache: source not found: {}", source_path.string());
        return nullptr;
    }
    const String source_text = src.read_all_text();

    const ShaderTarget target = detect_target();
    ShaderCompiler compiler(target);
    const Vector<ShaderStageOutput> stages = compiler.compile(source_text);
    if (stages.empty())
    {
        IG_CORE_ERROR("ShaderCache: compile failed for '{}'", source_path.string());
        return nullptr;
    }

    const Path cache_file = cache_file_for(path_hash, source_path);
    if (!write_disk(cache_file, content_hash, stages, target))
        IG_CORE_WARN("ShaderCache: failed to write cache for '{}'", source_path.string());
    else
        IG_CORE_INFO("ShaderCache: compiled '{}' -> {}", source_path.filename().string(),
                     cache_file.filename().string());

    SharedPtr<RenderShader> shader = make_render_shader(stages);
    if (shader)
        m_memory[path_hash] = {content_hash, shader};
    return shader;
}

// ---------------------------------------------------------------------------

void ShaderCache::remove(const Path& source_path)
{
    const uint64_t path_hash = hash_path(source_path);
    m_memory.erase(path_hash);

    const Path cache_file = cache_file_for(path_hash, source_path);
    if (Filesystem::exists(cache_file))
        Filesystem::remove(cache_file);
}

SharedPtr<RenderShader> ShaderCache::get_or_compile(const String& source_text, const String& virtual_name)
{
    const Path     virtual_path(virtual_name);
    const uint64_t path_hash    = hash_path(virtual_path);
    const uint64_t content_hash = fnv1a(source_text.data(), source_text.size());

    auto it = m_memory.find(path_hash);
    if (it != m_memory.end() && it->second.source_hash == content_hash)
        return it->second.shader;

    const Path cache_file = cache_file_for(path_hash, virtual_path);
    SharedPtr<RenderShader> loaded = try_load_disk(cache_file, content_hash);
    if (loaded)
    {
        m_memory[path_hash] = {content_hash, loaded};
        return loaded;
    }

    const ShaderTarget target = detect_target();
    ShaderCompiler compiler(target);
    const Vector<ShaderStageOutput> stages = compiler.compile(source_text);
    if (stages.empty())
    {
        IG_CORE_ERROR("ShaderCache: compile failed for inline shader '{}'", virtual_name);
        return nullptr;
    }

    if (!write_disk(cache_file, content_hash, stages, target))
        IG_CORE_WARN("ShaderCache: failed to write cache for inline shader '{}'", virtual_name);
    else
        IG_CORE_INFO("ShaderCache: compiled inline '{}' -> {}", virtual_name, cache_file.filename().string());

    SharedPtr<RenderShader> shader = make_render_shader(stages);
    if (shader)
        m_memory[path_hash] = {content_hash, shader};
    return shader;
}

SharedPtr<RenderShader> ShaderCache::get_or_compile(const Path& source_path)
{
    const uint64_t path_hash    = hash_path(source_path);
    const uint64_t content_hash = hash_content(source_path);

    // Memory hit — valid only when source hasn't changed
    auto it = m_memory.find(path_hash);
    if (it != m_memory.end() && it->second.source_hash == content_hash)
        return it->second.shader;

    // Disk hit
    const Path cache_file = cache_file_for(path_hash, source_path);
    SharedPtr<RenderShader> loaded = try_load_disk(cache_file, content_hash);
    if (loaded)
    {
        m_memory[path_hash] = {content_hash, loaded};
        return loaded;
    }

    // Cache miss or stale source — recompile
    return compile_and_store(source_path, path_hash, content_hash);
}

} // namespace Ignis
