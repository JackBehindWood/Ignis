#pragma once

#include "LayerStack.h"
#include "Window.h"
#include "Platform.h"

#include "Ignis/Events/Event.h"
#include "Ignis/Events/ApplicationEvent.h"
#include "Ignis/Foundation/Foundation.h"

#include <functional>

int main(int argc, char** argv);

namespace Ignis
{
struct ApplicationCommandLineArgs
{
    int32_t count = 0;
    char**  args  = nullptr;

    const char* operator[](int32_t index) const
    {
        IG_CORE_ASSERT(index < count);
        return args[index];
    }
};

struct ApplicationSpecification
{
    String                     name = "Ignis";
    String                     working_directory;
    uint32_t                   width  = 1280;
    uint32_t                   height = 720;
    ApplicationCommandLineArgs command_line_args;
};

class Application
{
private:
    static Application* s_instance;
    friend int ::main(int argc, char** argv);

    bool       m_running = true;
    LayerStack m_layer_stack;
    float      m_last_frame_time;

    Filesystem::path m_working_directory;

    Window m_window;

    void run();

public:
    Application(const ApplicationSpecification& specification);
    virtual ~Application();

    inline void close()
    {
        m_running = false;
    }

    inline void reset_frame_time()
    {
        m_last_frame_time = static_cast<float>(Platform::get_time());
    }

    void event(Event& e);

    void push_layer(Layer* layer);
    void push_overlay(Layer* layer);

    inline bool window_close(WindowCloseEvent& e)
    {
        m_running = false;
        return true;
    }

    inline bool window_resize(WindowResizeEvent& e)
    {
        m_window.resize(e.get_width(), e.get_height());
        return false;
    }

    static inline Application& get()
    {
        return *s_instance;
    }

    inline Window& get_window()
    {
        return m_window;
    }
};

// To be defined in CLIENT
Application* create_application(const ApplicationCommandLineArgs& spec);

} // namespace Ignis
