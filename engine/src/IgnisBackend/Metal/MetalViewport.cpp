#include "igpch.h"
#include "MetalViewport.h"

#include "MetalDevice.h"

#include "wrapper/wNSWindow.hpp"

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <QuartzCore/CAMetalLayer.hpp>

namespace Ignis
{
    MetalViewport::MetalViewport(MetalDevice* device, uint32_t width, uint32_t height) : m_width(width), m_height(height), m_metal_layer(nullptr), m_device(*device), m_window(nullptr)
    {
        const char* title = "Ignis Metal Window";
        m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    
        m_metal_layer = CA::MetalLayer::layer();
        m_metal_layer->setDevice(m_device.get_device());
        
        wNSWindow* nswindow = reinterpret_cast<wNSWindow*>(glfwGetCocoaWindow(m_window));
        
        wNSView* nsview = nswindow->content_view();
        nsview->set_layer(m_metal_layer);
        nsview->set_wants_layer(true);
        nsview->set_opaque(true);
    }

    MetalViewport::~MetalViewport()
    {
        glfwDestroyWindow(m_window);
    }

    void MetalViewport::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
    }
} // namespace Ignis
