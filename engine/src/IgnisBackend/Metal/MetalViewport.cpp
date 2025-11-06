#include "igpch.h"

#include "MetalResource.h"
#include "MetalGRI.h"
#include "MetalContext.h"

#include "Ignis/Core/Application.h"
#include "Ignis/Events/ApplicationEvent.h"
#include "Ignis/Events/KeyEvent.h"
#include "Ignis/Events/MouseEvent.h"

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

//#include <MetalKit/MetalKit.hpp>

namespace Ignis
{
    void MetalViewport::setup_callbacks()
    {
        glfwSetWindowUserPointer(m_window, this);

        glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int fb_width, int fb_height) 
        {
            MetalViewport* viewport = (MetalViewport*)glfwGetWindowUserPointer(window);
            WindowResizeEvent event(viewport, fb_width, fb_height);
            Application::get().event(event);
        });

        glfwSetWindowIconifyCallback(m_window, [](GLFWwindow* window, int iconified) 
        {
            MetalViewport* viewport = (MetalViewport*)glfwGetWindowUserPointer(window);

            int fb_width, fb_height;
            glfwGetFramebufferSize(window, &fb_width, &fb_height);

            // Send WindowResizeEvent even on minimize/restore
            WindowResizeEvent event(viewport, fb_width, fb_height);
            Application::get().event(event);
        });

        glfwSetWindowMaximizeCallback(m_window, [](GLFWwindow* window, int maximized) 
        {
            MetalViewport* viewport = (MetalViewport*)glfwGetWindowUserPointer(window);

            int fb_width, fb_height;
            glfwGetFramebufferSize(window, &fb_width, &fb_height);

            // Send WindowResizeEvent on maximize/restore
            WindowResizeEvent event(viewport, fb_width, fb_height);
            Application::get().event(event);
        });

        glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window)
        {
            MetalViewport* viewport = static_cast<MetalViewport*>(glfwGetWindowUserPointer(window));

            WindowCloseEvent event(viewport);
            Application::get().event(event);
        });

        glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            switch (action)
            {
                case GLFW_PRESS:
                {
                    KeyPressedEvent event(key, 0);
                    Application::get().event(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    KeyReleasedEvent event(key);
                    Application::get().event(event);
                    break;
                }
                case GLFW_REPEAT:
                {
                    KeyPressedEvent event(key, true);
                    Application::get().event(event);
                    break;
                }
            }
        });

		glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int keycode)
        {
            KeyTypedEvent event(keycode);
            Application::get().event(event);
        });

		glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods)
        {
            switch (action)
            {
                case GLFW_PRESS:
                {
                    MouseButtonPressedEvent event(button);
                    Application::get().event(event);
                    break;
                }
                
                case GLFW_RELEASE:
                {
                    MouseButtonReleasedEvent event(button);
                    Application::get().event(event);
                    break;
                }
            }
        });

		glfwSetScrollCallback(m_window, [](GLFWwindow* window, double x_offset, double y_offset)
        {
            MouseScrolledEvent event((float)x_offset, (float)y_offset);
            Application::get().event(event);
        });

		glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double x_pos, double y_pos)
        {
            MouseMovedEvent event((float)x_pos, (float)y_pos);
            Application::get().event(event);
        });
    }

    MetalViewport::MetalViewport(MetalDevice* device, const GRIViewportDesc& desc) : 
    m_width(desc.width), m_height(desc.height), m_metal_layer(nullptr), m_device(*device), m_window(nullptr), m_drawable(nullptr)
    {
        m_window = glfwCreateWindow(m_width, m_height, desc.title, nullptr, nullptr);
    
        m_metal_layer = CA::MetalLayer::layer();
        m_metal_layer->setDevice(m_device.get_device());
        m_metal_layer->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm); //TODO: Add support for other pixel formats
        
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

        release_drawable();
    }

    void MetalViewport::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
    }

    CA::MetalDrawable* MetalViewport::get_drawable()
    {
        if (!m_drawable)
        {
            m_drawable = m_metal_layer->nextDrawable();
            m_drawable->retain();
        }
        return m_drawable;
    }

    void MetalViewport::release_drawable()
    {
        if (m_drawable)
        {
            m_drawable->release();
            m_drawable = nullptr;
        }
    }

    GRIViewportPtr MetalGRI::create_viewport(const GRIViewportDesc& desc)
    {
        return create_unique<MetalViewport>(m_device, desc);
    }

    void MetalGRI::resize_viewport(GRIViewport* viewport, uint32_t width, uint32_t height)
	{
		MetalViewport* native_viewport = resource_cast(viewport);
		native_viewport->resize(width, height);
	}

    void MetalCommandContext::begin_drawing_viewport(GRIViewport* viewport, GRITexture2D* render_target)
    {
        MTL_AUTORELEASE_POOL;
        MetalViewport* native_viewport = resource_cast(viewport);

        m_state_cache.set_active_viewport(native_viewport);

        if (render_target)
        {
            GRIRenderTargetView rtv(render_target);
            set_render_targets(1, &rtv, nullptr);
        }
        else
        {
            GRIRenderTargetView rtv(nullptr);
            set_render_targets(0, &rtv, nullptr);
        }
    }

    void MetalCommandContext::end_drawing_viewport(GRIViewport* viewport)
    {
        MetalViewport* native_viewport = resource_cast(viewport);
        native_viewport->release_drawable();
    }
} // namespace Ignis
