#pragma once

#include "Ignis/Rendering/GRI/GRIViewport.h"

namespace Ignis
{
    class Window
    {
    private:
        GRIViewportPtr m_viewport;
    public:
        Window();
        ~Window() = default;
        
        void init(uint32_t width, uint32_t height);
        void update();

        inline GRIViewport* get_viewport() const { return m_viewport.get(); }
        inline uint32_t get_width() const { return m_viewport->get_width(); }
        inline uint32_t get_height() const { return m_viewport->get_height(); }

        void poll_events();
    };
}