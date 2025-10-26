#pragma once

#include "LayerStack.h"
#include "Timer.h"
#include "Window.h"

int main(int argc, char** argv);

namespace Ignis
{
    struct ApplicationCommandLineArgs
    {
        int32_t count = 0;
        char** args = nullptr;

        const char* operator[](int32_t index) const
        {
            IG_CORE_ASSERT(index < count);
            return args[index];
        }
    };

    struct ApplicationSpecification
    {
        String name = "Ignis Application";
        String working_directory;
        ApplicationCommandLineArgs command_line_args;
    };

    class Application
    {
    private:
        static Application* s_instance;
		friend int ::main(int argc, char** argv);

        ApplicationSpecification m_specification;
        bool m_running = true;
        LayerStack m_layer_stack;
        Timer m_timer;

        Window m_window;

        void run();
    public:
        Application(const ApplicationSpecification& specification);
		virtual ~Application();
        
        void close();
        void restart();

        void push_layer(Layer* layer);
		void push_overlay(Layer* layer);

        static inline Application& get() { return *s_instance; }

		inline const ApplicationSpecification& get_specification() const { return m_specification; }
    };

    // To be defined in CLIENT
    Application* create_application(const ApplicationCommandLineArgs& spec);

    
} // namespace Ignis

