#include "igpch.h"
#include "MetalViewport.h"

#include "Ignis/Core/Application.h"
#include "Ignis/Events/ApplicationEvent.h"

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
    void MetalViewport::setup_callbacks()
    {
        glfwSetWindowUserPointer(m_window, this);

        glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window)
        {
            MetalViewport* viewport = static_cast<MetalViewport*>(glfwGetWindowUserPointer(window));

            WindowCloseEvent event;
            Application::get().event(event);
        });
    }

    MetalViewport::MetalViewport(MetalDevice* device, const GRIViewportDesc& desc) : 
    m_width(desc.width), m_height(desc.height), m_metal_layer(nullptr), m_device(*device), m_window(nullptr)
    {
        m_window = glfwCreateWindow(m_width, m_height, desc.title, nullptr, nullptr);
    
        m_metal_layer = CA::MetalLayer::layer();
        m_metal_layer->setDevice(m_device.get_device());
        
        wNSWindow* nswindow = reinterpret_cast<wNSWindow*>(glfwGetCocoaWindow(m_window));
        
        wNSView* nsview = nswindow->content_view();
        nsview->set_layer(m_metal_layer);
        nsview->set_wants_layer(true);
        nsview->set_opaque(true);

        setup_callbacks();
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
