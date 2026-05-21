#include "igpch.h"
#include "ShaderLoader.h"
#include "AssetShader.h"
#include "AssetBinaryStream.h"
#include "AssetManager.h"
#include "Ignis/Rendering/ShaderCache.h"
#include "Ignis/Rendering/ShaderCompiler.h"


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

static bool resolve_from_recipe(AssetBinaryReader& r,
                                 SharedPtr<RenderShader>& out_vs, SharedPtr<RenderShader>& out_ps)
{
    const Path   source_path(read_str(r));
    const String entry_vs = read_str(r);
    const String entry_ps = read_str(r);
    const uint32_t num_defines = r.read_u32();

    ShaderCompilerOptions opts;
    opts.stages[0] = { GRIShaderStage::Vertex, entry_vs };
    opts.stages[1] = { GRIShaderStage::Pixel,  entry_ps };
    opts.count = 2;
    opts.defines.reserve(num_defines);
    for (uint32_t i = 0; i < num_defines; ++i)
    {
        String key = read_str(r);
        String val = read_str(r);
        opts.defines.push_back({ std::move(key), std::move(val) });
    }

    if (!r.good())
        return false;

    out_vs = ShaderCache::get().get_or_compile(source_path, GRIShaderStage::Vertex, opts);
    out_ps = ShaderCache::get().get_or_compile(source_path, GRIShaderStage::Pixel,  opts);
    return out_vs && out_ps;
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

    SharedPtr<RenderShader> vs, ps;
    if (!resolve_from_recipe(r, vs, ps))
    {
        IG_CORE_ERROR("ShaderLoader: shader compile failed for '{}'", metadata.source_path.string());
        return nullptr;
    }

    return create_shared<AssetShader>(metadata.ID, std::move(vs), std::move(ps));
}

} // namespace Ignis
