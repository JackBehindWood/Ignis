#include "igpch.h"
#include "AssetShaderCompiler.h"
#include "AssetBinaryStream.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"

#include <regex>
#include <chrono>

namespace Ignis
{

static constexpr AssetBlobHeader k_igas_v2_header = {{'I', 'G', 'A', 'S'}, 2};

static void write_str(AssetBinaryWriter& w, const String& s)
{
    w.write_u32(static_cast<uint32_t>(s.size()));
    if (!s.empty())
        w.write_bytes(s.data(), s.size());
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

static Vector<Path> scan_includes(const String& source, const Path& source_dir)
{
    Vector<Path> deps;
    static const std::regex k_include_re(R"re(#include\s*"([^"]+)")re");
    auto it  = std::sregex_iterator(source.begin(), source.end(), k_include_re);
    auto ite = std::sregex_iterator{};
    for (; it != ite; ++it)
    {
        Path resolved = source_dir / (*it)[1].str();
        std::error_code ec;
        resolved = Filesystem::canonical(resolved, ec);
        if (!ec)
            deps.push_back(std::move(resolved));
    }
    return deps;
}

bool AssetShaderCompiler::compile(const AssetMetadata& metadata)
{
    const GRIShaderStage stage = static_cast<GRIShaderStage>(metadata.user_data);

    const char* entry_point = nullptr;
    switch (stage)
    {
        case GRIShaderStage::Vertex:  entry_point = "VSMain"; break;
        case GRIShaderStage::Pixel:   entry_point = "PSMain"; break;
        case GRIShaderStage::Compute: entry_point = "CSMain"; break;
        default:
            IG_CORE_ERROR("AssetShaderCompiler: unknown stage for '{}'", metadata.source_path.string());
            return false;
    }

    String source;
    if (!read_source_text(metadata, source))
    {
        IG_CORE_ERROR("AssetShaderCompiler: failed to read source '{}'", metadata.source_path.string());
        return false;
    }

    const Path source_dir = metadata.source_path.parent_path();
    Vector<Path> includes  = scan_includes(source, source_dir);

    // dep manifest: source file + all includes
    Vector<std::pair<Path, uint64_t>> deps;
    deps.reserve(1 + includes.size());
    deps.push_back({ metadata.source_path, get_mtime_ns(metadata.source_path) });
    for (const Path& inc : includes)
        deps.push_back({ inc, get_mtime_ns(inc) });


    AssetBinaryWriter w = open_writer(metadata, k_igas_v2_header);
    if (!w.is_open())
    {
        IG_CORE_ERROR("AssetShaderCompiler: could not open output '{}'", metadata.compiled_path.string());
        return false;
    }

    write_str(w, metadata.source_path.string());
    w.write_u8(static_cast<uint8_t>(stage));
    write_str(w, entry_point);

    w.write_u32(0); // num_defines — reserved

    w.write_u32(static_cast<uint32_t>(deps.size()));
    for (const auto& [path, mtime] : deps)
    {
        write_str(w, path.string());
        w.write_u64(mtime);
    }

    return w.good();
}

} // namespace Ignis
