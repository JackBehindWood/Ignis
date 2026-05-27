#include "igpch.h"
#include "Window.h"

#include "Ignis/Rendering/RenderSystem.h"

namespace Ignis
{
namespace Utils
{
inline static bool is_minimal(uint32_t width, uint32_t height)
{
    return (width == 0 || height == 0);
}
} // namespace Utils

Window::Window()
    : m_viewport(nullptr),
      m_minimised(true)
{
}

void Window::init(const char* title, uint32_t width, uint32_t height)
{
    m_viewport  = RenderSystem::get_gri()->create_viewport({width, height, title});
    m_minimised = Utils::is_minimal(width, height);
}

void Window::update()
{
    // Update window-related events here (e.g., polling for input, resizing, etc.)
}

void Window::resize(uint32_t width, uint32_t height)
{
    m_minimised = Utils::is_minimal(width, height);

    if (!m_minimised)
    {
        RenderSystem::get_gri()->resize_viewport(m_viewport.get(), width, height);
    }
}
} // namespace Ignis