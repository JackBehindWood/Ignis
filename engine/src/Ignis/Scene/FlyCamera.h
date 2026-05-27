#pragma once

#include "Ignis/Scene/CameraData.h"
#include "Ignis/Core/MouseCodes.h"

namespace Ignis
{

class FlyCamera
{
public:
    void set_perspective(float fov_deg, float near_clip, float far_clip);
    void set_aspect(float aspect);

    void update(float ts);
    void on_mouse_scroll(float delta);
    void on_mouse_button(MouseCode button, bool pressed);
    void on_mouse_move(float x, float y);

    void focus_on(Math::Vec3f target, float distance);
    void set_position(Math::Vec3f pos);
    void set_orientation(float yaw_deg, float pitch_deg);

    CameraData get_camera_data() const;

private:
    Math::Vec3f m_position     = {0.0f, 0.0f, 3.0f};
    float       m_yaw          = -90.0f;
    float       m_pitch        = 0.0f;
    float       m_fov          = 60.0f;
    float       m_near         = 0.1f;
    float       m_far          = 1000.0f;
    float       m_aspect       = 1.778f;
    float       m_move_speed   = 5.0f;
    float       m_orbit_speed  = 0.3f;
    float       m_zoom_speed   = 1.0f;
    float       m_last_mouse_x = 0.0f;
    float       m_last_mouse_y = 0.0f;
    bool        m_right_held   = false;
};

} // namespace Ignis
