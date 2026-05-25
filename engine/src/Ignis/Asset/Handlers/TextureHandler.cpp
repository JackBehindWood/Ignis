#include "igpch.h"
#include "TextureHandler.h"

#include "Ignis/Asset/AssetTexture2D.h"
#include "Ignis/Asset/AssetBinaryStream.h"
#include "Ignis/Asset/AssetTypes.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace Ignis
{

static constexpr AssetBlobHeader k_texture_header = {{'I', 'G', 'T', 'X'}, 2};

template <typename R>
static SharedPtr<Asset> parse_texture_payload(const AssetMetadata& metadata, R& r)
{
    const AssetPixelFormat format = static_cast<AssetPixelFormat>(r.read_u8());
    const uint32_t         width  = r.read_u32();
    const uint32_t         height = r.read_u32();
    /* mip_levels = */ r.read_u8();
    const uint32_t pixel_bytes = r.read_u32();

    Vector<uint8_t> pixels(pixel_bytes);
    r.read_bytes(pixels.data(), pixel_bytes);

    if (!r.good())
    {
        IG_CORE_ERROR("TextureHandler: corrupted payload for '{}'", metadata.compiled_path.string());
        return nullptr;
    }

    return create_shared<AssetTexture2D>(metadata.ID, format, width, height, std::move(pixels));
}

// ---------------------------------------------------------------------------

bool TextureHandler::compile(const AssetMetadata& metadata)
{
    int      width, height, channels;
    stbi_uc* pixels = stbi_load(metadata.source_path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
    {
        IG_CORE_ERROR("TextureHandler: stbi_load failed for '{}'", metadata.source_path.string());
        return false;
    }

    AssetBinaryWriter w = open_writer(metadata, k_texture_header);
    if (!w.is_open())
    {
        stbi_image_free(pixels);
        IG_CORE_ERROR("TextureHandler: failed to open output '{}'", metadata.compiled_path.string());
        return false;
    }

    const uint32_t pixel_bytes = static_cast<uint32_t>(width) * static_cast<uint32_t>(height) * 4;

    w.write_u8(static_cast<uint8_t>(AssetPixelFormat::RGBA8Unorm));
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
            IG_CORE_ERROR("TextureHandler: cook failed for '{}'", metadata.source_path.string());
            return nullptr;
        }
        r = open_reader(metadata, k_texture_header);
        if (!r.is_open())
        {
            return nullptr;
        }
    }

    SharedPtr<Asset> asset = parse_texture_payload(metadata, r);
    if (!asset)
    {
        IG_CORE_ERROR("TextureHandler: corrupted binary for '{}'", metadata.compiled_path.string());
    }
    return asset;
}

SharedPtr<Asset> TextureHandler::load_from_bytes(const AssetMetadata& metadata, const Vector<uint8_t>& bytes)
{
    MemBinaryReader  r(bytes.data(), bytes.size());
    SharedPtr<Asset> asset = parse_texture_payload(metadata, r);
    if (!asset)
    {
        IG_CORE_ERROR("TextureHandler: corrupted payload for '{}'", metadata.compiled_path.string());
    }
    return asset;
}

AssetType TextureHandler::get_type() const
{
    return AssetType::Texture2D;
}

SharedPtr<Asset> TextureHandler::create_default_fallback() const
{
    constexpr uint32_t kW = 4, kH = 4;
    Vector<uint8_t>    pixels(kW * kH * 4);
    for (uint32_t row = 0; row < kH; ++row)
    {
        for (uint32_t col = 0; col < kW; ++col)
        {
            const bool mag = ((row + col) & 1) == 0;
            uint8_t*   p   = &pixels[(row * kW + col) * 4];
            p[0]           = mag ? 0xFF : 0x80;
            p[1]           = 0x00;
            p[2]           = mag ? 0xFF : 0x80;
            p[3]           = 0xFF;
        }
    }
    return create_shared<AssetTexture2D>(UUID{UUID::s_invalid}, AssetPixelFormat::RGBA8Unorm, kW, kH,
                                         std::move(pixels));
}

} // namespace Ignis
