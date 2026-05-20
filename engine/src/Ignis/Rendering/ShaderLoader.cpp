#include "igpch.h"
#include "ShaderLoader.h"
#include "ShaderCache.h"
#include "Ignis/Asset/AssetShader.h"
#include "Ignis/Asset/AssetBinaryStream.h"
#include "Ignis/Asset/AssetManager.h"

namespace Ignis
{

static constexpr AssetBlobHeader k_recipe_header = {{'I', 'G', 'A', 'S'}, 1};

static String read_str(AssetBinaryReader& r)
{
    const uint32_t len = r.read_u32();
    String s(len, '\0');
    if (len > 0)
        r.read_bytes(s.data(), len);
    return s;
}

// ---------------------------------------------------------------------------

static SharedPtr<RenderShader> resolve_from_recipe(AssetBinaryReader& r)
{
    const Path   source_path(read_str(r));
    const String entry_vs = read_str(r);   // stored for future per-asset entry point overrides
    const String entry_ps = read_str(r);
    const uint32_t num_defines = r.read_u32();
    for (uint32_t i = 0; i < num_defines * 2; ++i)
        read_str(r); // skip [name, value] pairs

    if (!r.good())
        return nullptr;

    return ShaderCache::get().get_or_compile(source_path);
}

// ---------------------------------------------------------------------------

SharedPtr<Asset> ShaderLoader::load(const AssetMetadata& metadata)
{
    // Try to open the IGAS v1 recipe
    AssetBinaryReader r = AssetBinaryReader::open(metadata.compiled_path, k_recipe_header);
    if (!r.is_open())
    {
        // Recipe missing — write it now (on-demand cook)
        AssetCompiler* compiler = AssetManager::get_compiler(AssetType::Shader);
        if (!compiler || !compiler->compile(metadata))
        {
            IG_CORE_ERROR("ShaderLoader: recipe cook failed for '{}'", metadata.source_path.string());
            return nullptr;
        }
        r = AssetBinaryReader::open(metadata.compiled_path, k_recipe_header);
        if (!r.is_open())
            return nullptr;
    }

    SharedPtr<RenderShader> rs = resolve_from_recipe(r);
    if (!rs)
    {
        IG_CORE_ERROR("ShaderLoader: shader compile failed for '{}'", metadata.source_path.string());
        return nullptr;
    }

    return create_shared<AssetShader>(metadata.ID, rs);
}

} // namespace Ignis
