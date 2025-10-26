#include "igpch.h"
#include "Application.h"
#include "Log.h"

#include "Ignis/Rendering/RenderSystem.h"

namespace Ignis
{
    Application* Application::s_instance = nullptr;

    Application::Application(const ApplicationSpecification& specification)
        : m_specification(specification)
    {
        IG_CORE_ASSERT(!s_instance, "Application already exists!");
        s_instance = this;

        if (m_specification.working_directory.empty())
        {
            m_specification.working_directory = std::filesystem::current_path().string();
        }
        std::filesystem::current_path(m_specification.working_directory);

        RenderSystem::init(GRIRenderAPI::Metal);

        m_window.init(1280, 720);
    }

    Application::~Application()
    {
        RenderSystem::shutdown();
    }

    void Application::close()
    {
        m_running = false;
    }


    void Application::restart()
    {
        close();
        run();
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
        m_timer.reset();
        while (m_running)
        {
            Timestep timestep = m_timer.elapsed();
            // Main application loop
            for (Layer* layer : m_layer_stack)
            {
                layer->update(timestep);
            }

            m_window.update();
            m_window.poll_events();

            m_timer.reset();
        }
    }
    
} // namespace Ignis