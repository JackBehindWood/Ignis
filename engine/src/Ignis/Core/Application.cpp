#include "igpch.h"
#include "Application.h"
#include "Log.h"
#include "Platform.h"

#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/Shaders/ShaderCache.h"

namespace Ignis
{
Application* Application::s_instance = nullptr;

Application::Application(const ApplicationSpecification& specification)
    : m_last_frame_time(0.0f)
{
    IG_CORE_ASSERT(!s_instance, "Application already exists!");
    s_instance = this;

    Filesystem::path working_directory = specification.working_directory;
    if (working_directory.empty())
    {
        working_directory = Filesystem::current_path();
    }

    Filesystem::current_path(working_directory);
    ShaderCache::get().set_cache_root(working_directory / "shadercache");

    RenderSystem::init(GRIRenderAPI::Metal);

    m_window.init(specification.name.c_str(), specification.width, specification.height);
}

Application::~Application()
{
    m_layer_stack.clear();
    RenderSystem::shutdown();
    s_instance = nullptr;
}

void Application::event(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.dispatch<WindowCloseEvent>(IG_BIND_EVENT_FN(Application::window_close));
    dispatcher.dispatch<WindowResizeEvent>(IG_BIND_EVENT_FN(Application::window_resize));

    for (Vector<Layer*>::reverse_iterator it = m_layer_stack.rbegin(); it != m_layer_stack.rend(); ++it)
    {
        (*it)->event(e);
        if (e.handled)
        {
            break;
        }
    }
}

void Application::push_layer(Layer* layer)
{
    m_layer_stack.push_layer(layer);
    layer->attach();
}

void Application::push_overlay(Layer* layer)
{
    m_layer_stack.push_overlay(layer);
    layer->attach();
}

void Application::run()
{
    m_last_frame_time = Platform::get_time();
    while (m_running)
    {
        float    time     = Platform::get_time();
        Timestep timestep = time - m_last_frame_time;
        m_last_frame_time = time;

        if (!m_window.is_minimised())
        {
            for (Layer* layer : m_layer_stack)
            {
                layer->update(timestep);
            }
        }
        m_window.update();
        Platform::poll_events();
    }
}

} // namespace Ignis
