#include "igpch.h"
#include "Application.h"
#include "Log.h"
#include "Platform.h"

#include "Ignis/Rendering/RenderSystem.h"


namespace Ignis
{
    Application* Application::s_instance = nullptr;

    Application::Application(const ApplicationSpecification& specification)
        : m_specification(specification), m_last_frame_time(0.0f)
    {
        IG_CORE_ASSERT(!s_instance, "Application already exists!");
        s_instance = this;

        if (m_specification.working_directory.empty())
        {
            m_specification.working_directory = std::filesystem::current_path().string();
        }
        std::filesystem::current_path(m_specification.working_directory);

        RenderSystem::init(GRIRenderAPI::Metal);

        m_window.init("Ignis",1280, 720);
    }

    Application::~Application()
    {
        RenderSystem::shutdown();
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

    bool Application::window_resize(WindowResizeEvent& e)
    {
        if (e.get_width() == 0 || e.get_height() == 0)
		{
			m_minimised = true;
			return false;
		}

		m_minimised = false;
		RenderSystem::get_gri()->resize_viewport(e.get_viewport(), e.get_width(), e.get_height());

		return false;
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
        while (m_running)
        {
            float time = Platform::get_time();
			Timestep timestep = time - m_last_frame_time;
			m_last_frame_time = time;
            // Main application loop
            if (!m_minimised)
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