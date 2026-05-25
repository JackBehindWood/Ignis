#pragma once

#include "Ignis/Asset/Asset.h"
#include "Ignis/Asset/AssetTypes.h"

namespace Ignis
{
class AssetTexture2D : public Asset
{
public:
    AssetTexture2D(AssetID id, AssetPixelFormat format, uint32_t width, uint32_t height, Vector<uint8_t> pixels)
        : m_format(format),
          m_width(width),
          m_height(height),
          m_pixels(std::move(pixels))
    {
        m_id = id;
    }

    AssetPixelFormat get_format() const
    {
        return m_format;
    }
    uint32_t get_width() const
    {
        return m_width;
    }
    uint32_t get_height() const
    {
        return m_height;
    }
    const Vector<uint8_t>& get_pixels() const
    {
        return m_pixels;
    }

    static AssetType static_type()
    {
        return AssetType::Texture2D;
    }

private:
    AssetPixelFormat m_format;
    uint32_t         m_width  = 0;
    uint32_t         m_height = 0;
    Vector<uint8_t>  m_pixels;
};

} // namespace Ignis
