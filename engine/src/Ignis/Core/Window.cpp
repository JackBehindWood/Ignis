#include "igpch.h"
#include "Window.h"

#include "Ignis/Rendering/RenderSystem.h"

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
} // namespace Ignis