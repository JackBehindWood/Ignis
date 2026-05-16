#include "MetalResource.h"

#include "MetalDevice.h"
#include "MetalGRI.h"

#include <Metal/Metal.hpp>

namespace Ignis
{
    MetalTexture2D::MetalTexture2D(MetalDevice* device, const GRITexture2DDesc& desc) : m_device(*device), m_width(desc.width), m_height(desc.height), m_mip_count(desc.num_mip_levels)
    {
        MTL::TextureDescriptor* tex_desc = MTL::TextureDescriptor::texture2DDescriptor(
            MTL::PixelFormat::PixelFormatBGRA8Unorm,
            desc.width,
            desc.height,
            desc.num_mip_levels > 1
        );

        tex_desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
        tex_desc->setMipmapLevelCount(desc.num_mip_levels);

        m_texture = m_device.get_device()->newTexture(tex_desc);
        tex_desc->release();
    }

    MetalTexture2D::MetalTexture2D(MetalDevice* device, MTL::Texture* texture, bool retain) : m_device(*device), m_texture(texture)
    {
        if (m_texture && retain)
        {
            m_texture->retain();
        }

        if (m_texture)
        {
            m_width = m_texture->width();
            m_height = m_texture->height();
            m_mip_count = m_texture->mipmapLevelCount();
        }
    }

    MetalTexture2D::~MetalTexture2D()
    {
        if (m_texture)
        {
            m_texture->release();
            m_texture = nullptr;
        }
    }

    GRITexture2DPtr MetalGRI::create_texture2d(const GRITexture2DDesc& desc)
    {
        return create_unique<MetalTexture2D>(m_device, desc);
    }
}