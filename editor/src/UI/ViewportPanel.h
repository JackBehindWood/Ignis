#pragma once

#include <Ignis/Scene/SceneRenderer.h>

namespace Ignis
{

class ViewportPanel
{
public:
    ViewportPanel() = default;

    void draw(SceneRenderer& scene_renderer);

    bool has_pending_resize() const
    {
        return m_resize_pending;
    }
    void flush_resize(SceneRenderer& scene_renderer);

private:
    uint32_t m_last_w         = 0;
    uint32_t m_last_h         = 0;
    uint32_t m_pending_w      = 0;
    uint32_t m_pending_h      = 0;
    bool     m_resize_pending = false;
};

} // namespace Ignis
