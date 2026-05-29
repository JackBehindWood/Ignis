#include "igpch.h"

#include "MetalResource.h"
#include "MetalGRI.h"
#include "MetalContext.h"

#include "Ignis/Core/Application.h"
#include "Ignis/Events/ApplicationEvent.h"
#include "Ignis/Events/KeyEvent.h"
#include "Ignis/Events/MouseEvent.h"

#include "MetalDevice.h"
#include "MetalCommandBuffer.h"

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

    glfwSetFramebufferSizeCallback(m_window,
                                   [](GLFWwindow* window, int fb_width, int fb_height)
                                   {
                                       MetalViewport*    viewport = (MetalViewport*)glfwGetWindowUserPointer(window);
                                       WindowResizeEvent event(viewport, fb_width, fb_height);
                                       Application::get().event(event);
                                   });

    glfwSetWindowIconifyCallback(m_window,
                                 [](GLFWwindow* window, int iconified)
                                 {
                                     MetalViewport* viewport = (MetalViewport*)glfwGetWindowUserPointer(window);

                                     int fb_width, fb_height;
                                     glfwGetFramebufferSize(window, &fb_width, &fb_height);

                                     WindowResizeEvent event(viewport, fb_width, fb_height);
                                     Application::get().event(event);
                                 });

    glfwSetWindowMaximizeCallback(m_window,
                                  [](GLFWwindow* window, int maximized)
                                  {
                                      MetalViewport* viewport = (MetalViewport*)glfwGetWindowUserPointer(window);

                                      int fb_width, fb_height;
                                      glfwGetFramebufferSize(window, &fb_width, &fb_height);

                                      WindowResizeEvent event(viewport, fb_width, fb_height);
                                      Application::get().event(event);
                                  });

    glfwSetWindowCloseCallback(m_window,
                               [](GLFWwindow* window)
                               {
                                   MetalViewport* viewport =
                                       static_cast<MetalViewport*>(glfwGetWindowUserPointer(window));

                                   WindowCloseEvent event(viewport);
                                   Application::get().event(event);
                               });

    glfwSetKeyCallback(m_window,
                       [](GLFWwindow* window, int key, int scancode, int action, int mods)
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

    glfwSetCharCallback(m_window,
                        [](GLFWwindow* window, unsigned int keycode)
                        {
                            KeyTypedEvent event(keycode);
                            Application::get().event(event);
                        });

    glfwSetMouseButtonCallback(m_window,
                               [](GLFWwindow* window, int button, int action, int mods)
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

    glfwSetScrollCallback(m_window,
                          [](GLFWwindow* window, double x_offset, double y_offset)
                          {
                              MouseScrolledEvent event((float)x_offset, (float)y_offset);
                              Application::get().event(event);
                          });

    glfwSetCursorPosCallback(m_window,
                             [](GLFWwindow* window, double x_pos, double y_pos)
                             {
                                 MouseMovedEvent event((float)x_pos, (float)y_pos);
                                 Application::get().event(event);
                             });
}

void MetalViewport::create_depth_buffer(uint32_t width, uint32_t height)
{
    destroy_depth_buffer();

    GRITexture2DDesc depth_desc;
    depth_desc.width          = width;
    depth_desc.height         = height;
    depth_desc.num_mip_levels = 1;
    depth_desc.format         = GRIPixelFormat::Depth32Float;

    m_depth_buffer = new MetalTexture2D(&m_device, depth_desc);
}

void MetalViewport::destroy_depth_buffer()
{
    if (m_depth_buffer)
    {
        delete m_depth_buffer;
        m_depth_buffer = nullptr;
    }
}

MetalViewport::MetalViewport(MetalDevice* device, const GRIViewportDesc& desc)
    : m_width(desc.width),
      m_height(desc.height),
      m_metal_layer(nullptr),
      m_device(*device),
      m_window(nullptr),
      m_drawable(nullptr),
      m_depth_buffer(nullptr)
{
    m_window = glfwCreateWindow(desc.width, desc.height, desc.title, nullptr, nullptr);

    // Use the physical framebuffer size — on Retina this is 2× the logical window size.
    // GLFW does not fire a framebuffer-size callback on initial creation, so querying here
    // is the only way to get the correct drawable dimensions before the first frame.
    int fb_w, fb_h;
    glfwGetFramebufferSize(m_window, &fb_w, &fb_h);
    m_width  = static_cast<uint32_t>(fb_w);
    m_height = static_cast<uint32_t>(fb_h);

    m_metal_layer = CA::MetalLayer::layer();
    m_metal_layer->setDevice(m_device.get_device());
    m_metal_layer->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm);
    m_metal_layer->setFramebufferOnly(true);
    m_metal_layer->setDrawableSize(CGSize((CGFloat)m_width, (CGFloat)m_height));
    m_metal_layer->setMaximumDrawableCount(2);

    wNSWindow* nswindow = reinterpret_cast<wNSWindow*>(glfwGetCocoaWindow(m_window));

    wNSView* nsview = nswindow->content_view();
    nsview->set_layer(m_metal_layer);
    nsview->set_wants_layer(true);
    nsview->set_opaque(true);

    setup_callbacks();
    create_depth_buffer(m_width, m_height);
}

MetalViewport::~MetalViewport()
{
    glfwDestroyWindow(m_window);
    release_drawable();
    destroy_depth_buffer();
}

void MetalViewport::set_current_drawable(CA::MetalDrawable* drawable)
{
    IG_CORE_ASSERT(!m_drawable, "set_current_drawable: previous drawable not released before acquiring next");
    m_drawable = drawable;
    m_drawable->retain();
    m_drawable_view.reset(drawable->texture());
}

void MetalViewport::resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    m_width  = width;
    m_height = height;

    if (m_metal_layer)
    {
        m_metal_layer->setDrawableSize(CGSize((CGFloat)width, (CGFloat)height));
    }

    // Release any in-flight drawable — its texture dimensions are now stale.
    // begin_drawing_viewport will acquire a fresh drawable before any RG pass runs.
    release_drawable();
    m_drawable_view.reset();

    create_depth_buffer(width, height);
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

GRITexture2D* MetalGRI::get_viewport_depth_texture(GRIViewport* viewport)
{
    return resource_cast(viewport)->get_depth_buffer();
}

void MetalCommandContext::begin_drawing_viewport(GRIViewport* viewport, GRITexture2D* render_target)
{
    m_active_viewport = resource_cast(viewport);

    CA::MetalDrawable* drawable = m_active_viewport->get_metal_layer()->nextDrawable();
    IG_CORE_ASSERT(drawable, "nextDrawable returned nil — GPU is too far ahead or drawable pool exhausted");
    m_active_viewport->set_current_drawable(drawable);

    GRITexture2D*            target = render_target ? render_target : m_active_viewport->get_drawable_view();
    GRIRenderTargetView      rtv(target);
    GRIDepthRenderTargetView depth_rtv(m_active_viewport->get_depth_buffer());
    set_render_targets(1, &rtv, &depth_rtv);
}
} // namespace Ignis
