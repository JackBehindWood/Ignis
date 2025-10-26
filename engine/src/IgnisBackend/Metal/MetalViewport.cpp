#include "igpch.h"
#include "MetalViewport.h"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <QuartzCore/CAMetalLayer.hpp>

namespace Ignis
{
    MetalViewport::MetalViewport(MTL::Device* device, uint32_t width, uint32_t height) : m_width(width), m_height(height)
    {
        // Initialize CAMetalLayer and NSView here
        m_metal_layer = CA::MetalLayer::layer();
        m_metal_layer->setDevice(device);
        m_metal_layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
        m_metal_layer->setFramebufferOnly(true);
        m_metal_layer->setDrawableSize(CGSizeMake(width, height));
    }


    MetalViewport::~MetalViewport()
    {
    }

    void MetalViewport::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
    }
} // namespace Ignis
