#pragma once

#include "Ignis/Rendering/GRI/GRIResource.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"

namespace Ignis
{
class RenderTexture2D : public RefCounted
{
public:
    RenderTexture2D(GRITexture2DPtr texture, uint32_t width, uint32_t height, GRIPixelFormat format)
        : m_texture(std::move(texture)),
          m_width(width),
          m_height(height),
          m_format(format)
    {
    }

    GRITexture2D* get_texture() const
    {
        return m_texture.get();
    }
    const GRITexture2DPtr& get_texture_ptr() const
    {
        return m_texture;
    }
    uint32_t get_width() const
    {
        return m_width;
    }
    uint32_t get_height() const
    {
        return m_height;
    }
    GRIPixelFormat get_format() const
    {
        return m_format;
    }

private:
    GRITexture2DPtr m_texture;
    uint32_t        m_width;
    uint32_t        m_height;
    GRIPixelFormat  m_format;
};
} // namespace Ignis
