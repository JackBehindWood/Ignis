#include "igpch.h"
#include "Window.h"

#include "Ignis/Rendering/RenderSystem.h"


#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

namespace Ignis
{
    Window::Window() : m_viewport(nullptr)
    {
    }

    void Window::init(uint32_t width, uint32_t height)
    {
        m_viewport = RenderSystem::get_gri()->create_viewport(width, height);
    }

    void Window::update()
    {
        // Update window-related events here (e.g., polling for input, resizing, etc.)
    }

    void Window::poll_events()
    {
        // Poll events from the underlying windowing system
        // This is a placeholder; actual implementation depends on the windowing library used
        glfwPollEvents();
    }
} // namespace Ignis