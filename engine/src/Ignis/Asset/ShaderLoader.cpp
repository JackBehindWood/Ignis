#include "igpch.h"
#include "ShaderLoader.h"
#include "AssetShader.h"
#include "AssetBinaryStream.h"
#include "AssetManager.h"
#include "Ignis/Rendering/Shaders/ShaderCache.h"
#include "Ignis/Rendering/Shaders/ShaderCompiler.h"

#include <chrono>

namespace Ignis
{

static constexpr AssetBlobHeader k_igas_v2_header = {{'I', 'G', 'A', 'S'}, 2};

static String read_str(AssetBinaryReader& r)
{
    const uint32_t len = r.read_u32();
    String s(len, '\0');
    if (len > 0)
        r.read_bytes(s.data(), len);
    return s;
}

static uint64_t get_mtime_ns(const Path& path)
{
    std::error_code ec;
    auto ftime = Filesystem::last_write_time(path, ec);
    if (ec)
        return 0;
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            ftime.time_since_epoch()).count());
}

static bool check_deps_stale(AssetBinaryReader& r, uint32_t num_deps)
{
    for (uint32_t i = 0; i < num_deps; ++i)
    {
        const Path     path(read_str(r));
        const uint64_t recorded_mtime = r.read_u64();

        if (Filesystem::exists(path) && get_mtime_ns(path) > recorded_mtime)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------

SharedPtr<Asset> ShaderLoader::load(const AssetMetadata& metadata)
{
    auto try_load = [&]() -> SharedPtr<Asset>
    {
        AssetBinaryReader r = AssetBinaryReader::open(metadata.compiled_path, k_igas_v2_header);
        if (!r.is_open())
            return nullptr;

        const Path   source_path(read_str(r));
        const auto   stage       = static_cast<GRIShaderStage>(r.read_u8());
        const String entry_point = read_str(r);
        const uint32_t num_defines = r.read_u32();

        ShaderCompilerOptions opts;
        opts.stages[static_cast<size_t>(stage)] = { stage, entry_point };
        opts.count = 1;
        opts.defines.reserve(num_defines);
        for (uint32_t i = 0; i < num_defines; ++i)
        {
            String key = read_str(r);
            String val = read_str(r);
            opts.defines.push_back({ std::move(key), std::move(val) });
        }

        const uint32_t num_deps = r.read_u32();
        if (check_deps_stale(r, num_deps))
            return nullptr; // signal recompile needed

        if (!r.good())
            return nullptr;

        SharedPtr<RenderShader> rs = ShaderCache::get().get_or_compile(source_path, stage, opts);
        if (!rs)
            return nullptr;

        return create_shared<AssetShader>(metadata.ID, std::move(rs));
    };

    // First attempt
    SharedPtr<Asset> asset = try_load();
    if (asset)
        return asset;

    // Recipe missing or deps stale — recompile
    AssetCompiler* compiler = AssetManager::get_compiler(AssetType::Shader);
    if (!compiler || !compiler->compile(metadata))
    {
        IG_CORE_ERROR("ShaderLoader: cook failed for '{}'", metadata.source_path.string());
        return nullptr;
    }

    asset = try_load();
    if (!asset)
        IG_CORE_ERROR("ShaderLoader: shader compile failed for '{}'", metadata.source_path.string());

    return asset;
}

} // namespace Ignis
