#include "igpch.h"
#include "ShaderHandler.h"

#include "Ignis/Asset/AssetShader.h"
#include "Ignis/Asset/AssetBinaryStream.h"
#include "Ignis/Asset/AssetTypes.h"

namespace Ignis
{

static constexpr AssetBlobHeader k_shader_header = {{'I', 'G', 'A', 'S'}, 2};

static Vector<Path> scan_includes(const String& source, const Path& source_dir)
{
    Vector<Path>            deps;
    static const std::regex k_include_re(R"re(#include\s*"([^"]+)")re");
    auto                    it  = std::sregex_iterator(source.begin(), source.end(), k_include_re);
    auto                    ite = std::sregex_iterator{};
    for (; it != ite; ++it)
    {
        Path            resolved = source_dir / (*it)[1].str();
        std::error_code ec;
        resolved = Filesystem::canonical(resolved, ec);
        if (!ec)
        {
            deps.push_back(std::move(resolved));
        }
    }
    return deps;
}

template <typename R>
static SharedPtr<Asset> parse_shader_payload(const AssetMetadata& metadata, R& r)
{
    const Path             source_path(ShaderHandler::read_string(r));
    const AssetShaderStage stage       = static_cast<AssetShaderStage>(r.read_u8());
    const String           entry_point = ShaderHandler::read_string(r);
    const uint32_t         num_defines = r.read_u32();

    for (uint32_t i = 0; i < num_defines; ++i)
    {
        ShaderHandler::read_string(r); // key
        ShaderHandler::read_string(r); // value
    }

    const uint32_t num_deps = r.read_u32();
    bool           stale    = false;
    for (uint32_t i = 0; i < num_deps; ++i)
    {
        const Path     dep_path(ShaderHandler::read_string(r));
        const uint64_t recorded_mtime = r.read_u64();
        if (!stale && Filesystem::exists(dep_path) && AssetHandler::get_file_mtime_ns(dep_path) > recorded_mtime)
        {
            stale = true;
        }
    }

    if (!r.good() || stale)
    {
        IG_CORE_ERROR("ShaderHandler: stale or corrupted payload for '{}'", metadata.compiled_path.string());
        return nullptr;
    }

    return create_shared<AssetShader>(metadata.ID, source_path, stage, entry_point);
}

// ---------------------------------------------------------------------------

bool ShaderHandler::compile(const AssetMetadata& metadata)
{
    const AssetShaderStage stage = static_cast<AssetShaderStage>(metadata.user_data);

    const char* entry_point = nullptr;
    switch (stage)
    {
        case AssetShaderStage::Vertex:
            entry_point = "VSMain";
            break;
        case AssetShaderStage::Pixel:
            entry_point = "PSMain";
            break;
        case AssetShaderStage::Compute:
            entry_point = "CSMain";
            break;
        default:
            IG_CORE_ERROR("ShaderHandler: unknown stage for '{}'", metadata.source_path.string());
            return false;
    }

    String source;
    if (!read_source_text(metadata, source))
    {
        return false;
    }

    const Path   source_dir = metadata.source_path.parent_path();
    Vector<Path> includes   = scan_includes(source, source_dir);

    Vector<std::pair<Path, uint64_t>> deps;
    deps.reserve(1 + includes.size());
    deps.push_back({metadata.source_path, get_file_mtime_ns(metadata.source_path)});
    for (const Path& inc : includes)
    {
        deps.push_back({inc, get_file_mtime_ns(inc)});
    }

    AssetBinaryWriter w = open_writer(metadata, k_shader_header);
    if (!w.is_open())
    {
        IG_CORE_ERROR("ShaderHandler: could not open output '{}'", metadata.compiled_path.string());
        return false;
    }

    write_string(w, metadata.source_path.string());
    w.write_u8(static_cast<uint8_t>(stage));
    write_string(w, entry_point);
    w.write_u32(0); // num_defines — reserved

    w.write_u32(static_cast<uint32_t>(deps.size()));
    for (const auto& [path, mtime] : deps)
    {
        write_string(w, path.string());
        w.write_u64(mtime);
    }

    return w.finalize();
}

SharedPtr<Asset> ShaderHandler::load(const AssetMetadata& metadata)
{
    auto try_load = [&]() -> SharedPtr<Asset>
    {
        AssetBinaryReader r = AssetBinaryReader::open(metadata.compiled_path, k_shader_header);
        if (!r.is_open())
        {
            return nullptr;
        }
        return parse_shader_payload(metadata, r);
    };

    SharedPtr<Asset> asset = try_load();
    if (asset)
    {
        return asset;
    }

    if (!compile(metadata))
    {
        IG_CORE_ERROR("ShaderHandler: cook failed for '{}'", metadata.source_path.string());
        return nullptr;
    }

    asset = try_load();
    if (!asset)
    {
        IG_CORE_ERROR("ShaderHandler: load after cook failed for '{}'", metadata.source_path.string());
    }
    return asset;
}

SharedPtr<Asset> ShaderHandler::load_from_bytes(const AssetMetadata& metadata, const Vector<uint8_t>& bytes)
{
    MemBinaryReader  r(bytes.data(), bytes.size());
    SharedPtr<Asset> asset = parse_shader_payload(metadata, r);
    if (!asset)
    {
        if (compile(metadata))
        {
            asset = load(metadata);
        }
        if (!asset)
        {
            IG_CORE_ERROR("ShaderHandler: recompile failed for '{}'", metadata.source_path.string());
        }
    }
    return asset;
}

AssetType ShaderHandler::get_type() const
{
    return AssetType::Shader;
}
SharedPtr<Asset> ShaderHandler::create_default_fallback() const
{
    return nullptr;
}

} // namespace Ignis
