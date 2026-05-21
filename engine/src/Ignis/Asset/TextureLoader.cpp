#include "igpch.h"
#include "TextureLoader.h"
#include "AssetTexture2D.h"
#include "AssetManager.h"
#include "AssetBinaryStream.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/RenderTexture2D.h"

namespace Ignis
{

static constexpr AssetBlobHeader k_texture_header = {{'I', 'G', 'T', 'X'}, 1};

SharedPtr<Asset> TextureLoader::load(const AssetMetadata& metadata)
{
    AssetBinaryReader r = AssetBinaryReader::open(metadata.compiled_path, k_texture_header);
    if (!r.is_open())
    {
        AssetCompiler* compiler = AssetManager::get_compiler(AssetType::Texture2D);
        if (!compiler || !compiler->compile(metadata))
        {
            IG_CORE_ERROR("TextureLoader: cook failed for '{}'", metadata.source_path.string());
            return nullptr;
        }
        r = AssetBinaryReader::open(metadata.compiled_path, k_texture_header);
        if (!r.is_open())
            return nullptr;
    }

    const auto     format      = static_cast<GRIPixelFormat>(r.read_u8());
    const uint32_t width       = r.read_u32();
    const uint32_t height      = r.read_u32();
    /* mip_levels */ r.read_u8();
    const uint32_t pixel_bytes = r.read_u32();

    Vector<uint8_t> pixels(pixel_bytes);
    r.read_bytes(pixels.data(), pixel_bytes);

    if (!r.good())
    {
        IG_CORE_ERROR("TextureLoader: corrupted binary for '{}'", metadata.compiled_path.string());
        return nullptr;
    }

    GRITexture2DDesc desc;
    desc.width             = width;
    desc.height            = height;
    desc.num_mip_levels    = 1;
    desc.format            = format;
    desc.initial_data      = pixels.data();
    desc.initial_data_size = pixel_bytes;

    GRITexture2DPtr tex = RenderSystem::get_gri()->create_texture2d(desc);
    if (!tex)
    {
        IG_CORE_ERROR("TextureLoader: GRI create_texture2d failed for '{}'", metadata.source_path.string());
        return nullptr;
    }

    auto rt = create_shared<RenderTexture2D>(std::move(tex), width, height, format);
    return create_shared<AssetTexture2D>(metadata.ID, std::move(rt));
}

} // namespace Ignis
