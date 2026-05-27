#pragma once

#include "Ignis/Rendering/GRI/GRIResource.h"

namespace Ignis
{
class Window
{
private:
    GRIViewportPtr m_viewport;
    bool           m_minimised;

public:
    Window();
    ~Window() = default;

    void init(const char* title, uint32_t width, uint32_t height);
    void update();
    void resize(uint32_t width, uint32_t height);

    inline GRIViewport* get_viewport() const
    {
        return m_viewport.get();
    }
    inline uint32_t get_width() const
    {
        return m_viewport->get_width();
    }
    inline uint32_t get_height() const
    {
        return m_viewport->get_height();
    }
    inline bool is_minimised() const
    {
        return m_minimised;
    }
};
} // namespace Ignis