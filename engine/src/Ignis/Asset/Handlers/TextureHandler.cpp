#include "igpch.h"
#include "TextureHandler.h"

#include "Ignis/Asset/AssetTexture2D.h"
#include "Ignis/Asset/AssetBinaryStream.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/RenderTexture2D.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace Ignis
{

static constexpr AssetBlobHeader k_texture_header = {{'I', 'G', 'T', 'X'}, 2};

template<typename R>
static SharedPtr<Asset> parse_texture_payload(const AssetMetadata& metadata, R& r)
{
    const GRIPixelFormat format     = static_cast<GRIPixelFormat>(r.read_u8());
    const uint32_t       width      = r.read_u32();
    const uint32_t       height     = r.read_u32();
    /* mip_levels = */               r.read_u8();
    const uint32_t       pixel_bytes = r.read_u32();

    Vector<uint8_t> pixels(pixel_bytes);
    r.read_bytes(pixels.data(), pixel_bytes);

    if (!r.good())
    {
        IG_CORE_ERROR("TextureHandler: corrupted payload for '{}'",
                      metadata.compiled_path.string());
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
        IG_CORE_ERROR("TextureHandler: GRI create_texture2d failed for '{}'",
                      metadata.source_path.string());
        return nullptr;
    }

    auto rt = create_shared<RenderTexture2D>(std::move(tex), width, height, format);
    return create_shared<AssetTexture2D>(metadata.ID, std::move(rt));
}

// ---------------------------------------------------------------------------

bool TextureHandler::compile(const AssetMetadata& metadata)
{
    int width, height, channels;
    stbi_uc* pixels = stbi_load(metadata.source_path.string().c_str(),
                                &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
    {
        IG_CORE_ERROR("TextureHandler: stbi_load failed for '{}'",
                      metadata.source_path.string());
        return false;
    }

    AssetBinaryWriter w = open_writer(metadata, k_texture_header);
    if (!w.is_open())
    {
        stbi_image_free(pixels);
        IG_CORE_ERROR("TextureHandler: failed to open output '{}'",
                      metadata.compiled_path.string());
        return false;
    }

    const uint32_t pixel_bytes = static_cast<uint32_t>(width) *
                                  static_cast<uint32_t>(height) * 4;

    w.write_u8(static_cast<uint8_t>(GRIPixelFormat::RGBA8Unorm));
    w.write_u32(static_cast<uint32_t>(width));
    w.write_u32(static_cast<uint32_t>(height));
    w.write_u8(1); // mip_levels
    w.write_u32(pixel_bytes);
    w.write_bytes(pixels, pixel_bytes);

    stbi_image_free(pixels);
    return w.finalize();
}

SharedPtr<Asset> TextureHandler::load(const AssetMetadata& metadata)
{
    AssetBinaryReader r = open_reader(metadata, k_texture_header);
    if (!r.is_open())
    {
        if (!compile(metadata))
        {
            IG_CORE_ERROR("TextureHandler: cook failed for '{}'",
                          metadata.source_path.string());
            return nullptr;
        }
        r = open_reader(metadata, k_texture_header);
        if (!r.is_open()) return nullptr;
    }

    SharedPtr<Asset> asset = parse_texture_payload(metadata, r);
    if (!asset)
        IG_CORE_ERROR("TextureHandler: corrupted binary for '{}'",
                      metadata.compiled_path.string());
    return asset;
}

SharedPtr<Asset> TextureHandler::load_from_bytes(const AssetMetadata& metadata,
                                                   const Vector<uint8_t>& bytes)
{
    MemBinaryReader r(bytes.data(), bytes.size());
    SharedPtr<Asset> asset = parse_texture_payload(metadata, r);
    if (!asset)
        IG_CORE_ERROR("TextureHandler: corrupted payload for '{}'",
                      metadata.compiled_path.string());
    return asset;
}

} // namespace Ignis
