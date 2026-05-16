#include <igpch.h>
#include "MetalResource.h"

#include "MetalDevice.h"
#include "MetalGRI.h"

#include <Metal/Metal.hpp>

namespace Ignis
{
    static MTL::PixelFormat to_metal_pixel_format(GRIPixelFormat fmt)
    {
        switch (fmt)
        {
            case GRIPixelFormat::RGBA8Unorm:    return MTL::PixelFormatRGBA8Unorm;
            case GRIPixelFormat::BGRA8Unorm:    return MTL::PixelFormatBGRA8Unorm;
            case GRIPixelFormat::Depth32Float:  return MTL::PixelFormatDepth32Float;
            default:                            return MTL::PixelFormatBGRA8Unorm;
        }
    }

    static bool is_depth_format(GRIPixelFormat fmt)
    {
        return fmt == GRIPixelFormat::Depth32Float;
    }

    MetalTexture2D::MetalTexture2D(MetalDevice* device, const GRITexture2DDesc& desc) : m_device(*device), m_width(desc.width), m_height(desc.height), m_mip_count(desc.num_mip_levels)
    {
        MTL::PixelFormat mtl_format = to_metal_pixel_format(desc.format);

        MTL::TextureDescriptor* tex_desc = MTL::TextureDescriptor::texture2DDescriptor(
            mtl_format,
            desc.width,
            desc.height,
            desc.num_mip_levels > 1
        );

        if (is_depth_format(desc.format))
            tex_desc->setUsage(MTL::TextureUsageRenderTarget);
        else
            tex_desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);

        tex_desc->setMipmapLevelCount(desc.num_mip_levels);

        m_texture = m_device.get_device()->newTexture(tex_desc);
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
        return create_shared<MetalTexture2D>(m_device, desc);
    }
}