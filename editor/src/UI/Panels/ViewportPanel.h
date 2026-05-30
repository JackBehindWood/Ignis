#pragma once

#include "IPanel.h"

namespace Ignis
{

class ViewportPanel : public IPanel
{
public:
    ViewportPanel() = default;

    PanelId     get_id() const override;
    const char* get_title() const override
    {
        return "Viewport";
    }
    void draw(IWorkspaceData* ctx) override;

    bool has_pending_resize() const override
    {
        return m_resize_pending;
    }
    void flush_resize(IWorkspaceData* ctx) override;

    void             push_window_style() override;
    void             pop_window_style() override;
    ImGuiWindowFlags get_window_flags() const override;

private:
    uint32_t m_last_w         = 0;
    uint32_t m_last_h         = 0;
    uint32_t m_pending_w      = 0;
    uint32_t m_pending_h      = 0;
    bool     m_resize_pending = false;
};

} // namespace Ignis
