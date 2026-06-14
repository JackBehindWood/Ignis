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
        case GRIPixelFormat::RGBA8Unorm:
            return MTL::PixelFormatRGBA8Unorm;
        case GRIPixelFormat::BGRA8Unorm:
            return MTL::PixelFormatBGRA8Unorm;
        case GRIPixelFormat::RGBA16Float:
            return MTL::PixelFormatRGBA16Float;
        case GRIPixelFormat::Depth32Float:
            return MTL::PixelFormatDepth32Float;
        case GRIPixelFormat::RG16Float:
            return MTL::PixelFormatRG16Float;
        default:
            return MTL::PixelFormatBGRA8Unorm;
    }
}

static uint32_t bytes_per_pixel(GRIPixelFormat fmt)
{
    switch (fmt)
    {
        case GRIPixelFormat::RGBA8Unorm:
            return 4;
        case GRIPixelFormat::BGRA8Unorm:
            return 4;
        case GRIPixelFormat::RGBA16Float:
            return 8;
        case GRIPixelFormat::Depth32Float:
            return 4;
        case GRIPixelFormat::RG16Float:
            return 4;
        default:
            return 4;
    }
}

static uint32_t bytes_per_pixel_mtl(MTL::PixelFormat fmt)
{
    switch (fmt)
    {
        case MTL::PixelFormatRGBA8Unorm:
            return 4;
        case MTL::PixelFormatBGRA8Unorm:
            return 4;
        case MTL::PixelFormatRGBA16Float:
            return 8;
        case MTL::PixelFormatDepth32Float:
            return 4;
        case MTL::PixelFormatRG16Float:
            return 4;
        default:
            return 4;
    }
}

static bool is_depth_format(GRIPixelFormat fmt)
{
    return fmt == GRIPixelFormat::Depth32Float;
}

MetalTexture2D::MetalTexture2D(MetalDevice* device, const GRITexture2DDesc& desc)
    : m_device(*device),
      m_width(desc.width),
      m_height(desc.height),
      m_mip_count(desc.num_mip_levels)
{
    MTL::PixelFormat mtl_format = to_metal_pixel_format(desc.format);

    MTL::TextureDescriptor* tex_desc =
        MTL::TextureDescriptor::texture2DDescriptor(mtl_format, desc.width, desc.height, desc.num_mip_levels > 1);

    if (is_depth_format(desc.format))
    {
        tex_desc->setUsage(MTL::TextureUsageRenderTarget);
    }
    else
    {
        MTL::TextureUsage usage = MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead;
        if (desc.allow_unordered_access)
        {
            usage |= MTL::TextureUsageShaderWrite;
        }
        tex_desc->setUsage(usage);
    }

    tex_desc->setMipmapLevelCount(desc.num_mip_levels);

    m_texture = m_device.get_device()->newTexture(tex_desc);

    if (desc.initial_data)
    {
        const uint32_t bpr    = desc.width * bytes_per_pixel(desc.format);
        MTL::Region    region = MTL::Region::Make2D(0, 0, desc.width, desc.height);
        m_texture->replaceRegion(region, 0, desc.initial_data, bpr);
    }
}

MetalTexture2D::MetalTexture2D(MetalDevice* device, MTL::Texture* texture, bool retain)
    : m_device(*device),
      m_texture(texture)
{
    if (m_texture && retain)
    {
        m_texture->retain();
    }

    if (m_texture)
    {
        m_width     = m_texture->width();
        m_height    = m_texture->height();
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

void MetalTexture2D::bind_storage(MTL::ComputeCommandEncoder* encoder, uint32_t slot, uint32_t mip_level,
                                  uint32_t array_slice) const
{
    if (mip_level == 0 && array_slice == 0)
    {
        encoder->setTexture(m_texture, slot);
        return;
    }
    // Create a transient 2D view targeting a specific mip + array slice (or cubemap face).
    // setTexture retains the view internally; release here is safe and required to avoid leaking.
    MTL::Texture* view = m_texture->newTextureView(m_texture->pixelFormat(), MTL::TextureType2D,
                                                   NS::Range::Make(mip_level, 1), NS::Range::Make(array_slice, 1));
    encoder->setTexture(view, slot);
    view->release();
}

GRITexture2DPtr MetalGRI::create_texture2d(const GRITexture2DDesc& desc)
{
    return create_shared<MetalTexture2D>(m_device, desc);
}

GRISamplerStatePtr MetalGRI::create_sampler_state(const GRISamplerDesc& desc)
{
    MTL::SamplerDescriptor* sd = MTL::SamplerDescriptor::alloc()->init();

    MTL::SamplerMinMagFilter filter =
        desc.linear_filter ? MTL::SamplerMinMagFilterLinear : MTL::SamplerMinMagFilterNearest;
    sd->setMinFilter(filter);
    sd->setMagFilter(filter);
    sd->setMipFilter(desc.linear_filter ? MTL::SamplerMipFilterLinear : MTL::SamplerMipFilterNearest);

    MTL::SamplerAddressMode address =
        desc.clamp_to_edge ? MTL::SamplerAddressModeClampToEdge : MTL::SamplerAddressModeRepeat;
    sd->setSAddressMode(address);
    sd->setTAddressMode(address);
    sd->setRAddressMode(address);

    MTL::SamplerState* sampler = m_device->get_device()->newSamplerState(sd);
    sd->release();

    IG_CORE_ASSERT(sampler, "MetalGRI: failed to create sampler state");
    return create_shared<MetalSamplerState>(sampler);
}

void MetalGRI::read_texture_sync(GRITexture2D* tex, uint32_t face, uint32_t mip, Vector<uint8_t>& out)
{
    MetalTexture2D* mtl   = static_cast<MetalTexture2D*>(tex);
    uint32_t        w     = std::max(1u, mtl->get_width() >> mip);
    uint32_t        h     = std::max(1u, mtl->get_height() >> mip);
    uint32_t        bpp   = bytes_per_pixel_mtl(mtl->get_texture()->pixelFormat());
    uint32_t        pitch = w * bpp;
    size_t          total = static_cast<size_t>(pitch) * h;

    MTL::Buffer* staging = m_device->get_device()->newBuffer(total, MTL::ResourceStorageModeShared);
    IG_CORE_ASSERT(staging, "MetalGRI: failed to allocate readback staging buffer");

    // Use the compute queue so readback is serialized after any compute writes on that same queue.
    MTL::CommandBuffer*      blit_cb = m_device->compute_queue().get_queue()->commandBuffer();
    MTL::BlitCommandEncoder* enc     = blit_cb->blitCommandEncoder();

    enc->copyFromTexture(mtl->get_texture(), face, mip, MTL::Origin{0, 0, 0}, MTL::Size{w, h, 1}, staging,
                         /*destinationOffset=*/0,
                         /*destinationBytesPerRow=*/pitch,
                         /*destinationBytesPerImage=*/0);

    enc->endEncoding();
    blit_cb->commit();
    // Wait for GPU completion before reading shared memory.
    blit_cb->waitUntilCompleted();

    out.resize(total);
    memcpy(out.data(), staging->contents(), total);

    staging->release();
    blit_cb->release();
}

GRITexture2DPtr MetalGRI::create_cubemap(const GRITexture2DDesc& desc)
{
    IG_CORE_ASSERT(desc.width == desc.height, "Cubemap faces must be square");
    IG_CORE_ASSERT(desc.width > 0, "Cubemap face size must be > 0");

    MTL::PixelFormat        mtl_format = to_metal_pixel_format(desc.format);
    MTL::TextureDescriptor* td         = MTL::TextureDescriptor::alloc()->init();

    td->setTextureType(MTL::TextureTypeCube);
    td->setPixelFormat(mtl_format);
    td->setWidth(desc.width);
    td->setHeight(desc.height);
    td->setMipmapLevelCount(desc.num_mip_levels);
    td->setArrayLength(1);
    MTL::TextureUsage cube_usage = MTL::TextureUsageShaderRead | MTL::TextureUsageRenderTarget;
    if (desc.allow_unordered_access)
    {
        cube_usage |= MTL::TextureUsageShaderWrite;
    }
    td->setUsage(cube_usage);
    td->setStorageMode(MTL::StorageModePrivate);

    MTL::Texture* texture = m_device->get_device()->newTexture(td);
    td->release();

    IG_CORE_ASSERT(texture, "MetalGRI: failed to allocate cubemap texture");
    return create_shared<MetalTexture2D>(m_device, texture, false);
}
} // namespace Ignis
