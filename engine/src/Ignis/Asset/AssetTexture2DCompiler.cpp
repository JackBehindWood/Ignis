#include "igpch.h"
#include "AssetTexture2DCompiler.h"
#include "AssetBinaryStream.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace Ignis
{

static constexpr AssetBlobHeader k_texture_header = {{'I', 'G', 'T', 'X'}, 1};

bool AssetTexture2DCompiler::compile(const AssetMetadata& metadata)
{
    int width, height, channels;
    stbi_uc* pixels = stbi_load(metadata.source_path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
    {
        IG_CORE_ERROR("AssetTexture2DCompiler: stbi_load failed for '{}'", metadata.source_path.string());
        return false;
    }

    AssetBinaryWriter w = AssetBinaryWriter::open(metadata.compiled_path, k_texture_header);
    if (!w.is_open())
    {
        stbi_image_free(pixels);
        IG_CORE_ERROR("AssetTexture2DCompiler: failed to open output '{}'", metadata.compiled_path.string());
        return false;
    }

    const uint32_t pixel_bytes = static_cast<uint32_t>(width) * static_cast<uint32_t>(height) * 4;

    w.write_u8(static_cast<uint8_t>(GRIPixelFormat::RGBA8Unorm));
    w.write_u32(static_cast<uint32_t>(width));
    w.write_u32(static_cast<uint32_t>(height));
    w.write_u8(1); // mip_levels
    w.write_u32(pixel_bytes);
    w.write_bytes(pixels, pixel_bytes);

    stbi_image_free(pixels);
    return w.good();
}

} // namespace Ignis
