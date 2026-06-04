#pragma once

#include "IPanel.h"
#include <Ignis/Scene/FlyCamera.h>
#include <Ignis/Math/Transform.h>

namespace Ignis
{

class ViewportPanel : public IPanel
{
public:
    ViewportPanel();

    PanelId     get_id() const override;
    const char* get_title() const override
    {
        return "Viewport";
    }

    void update(float ts, IWorkspaceData* ctx) override;
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
    FlyCamera m_fly_camera;
    bool      m_hovered        = false;
    uint32_t  m_last_w         = 0;
    uint32_t  m_last_h         = 0;
    uint32_t  m_pending_w      = 0;
    uint32_t  m_pending_h      = 0;
    bool      m_resize_pending = false;

    // Orient widget snap state (set in draw(), consumed in update())
    int   m_orient_clicked    = -1;
    float m_snap_t            = 1.0f; // 1.0 = no active snap
    float m_snap_yaw_start    = 0.0f;
    float m_snap_yaw_target   = 0.0f;
    float m_snap_pitch_start  = 0.0f;
    float m_snap_pitch_target = 0.0f;

    Math::Transformf m_gizmo_before     = {};
    bool             m_gizmo_was_active = false;
};

} // namespace Ignis
