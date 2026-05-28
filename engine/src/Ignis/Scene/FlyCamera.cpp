#include "igpch.h"
#include "Ignis/Scene/FlyCamera.h"
#include "Ignis/Core/Input.h"
#include "Ignis/Core/KeyCodes.h"
#include "Ignis/Core/MouseCodes.h"
#include "FlyCamera.h"

namespace Ignis
{

namespace Utils
{
Math::Vec3f compute_forward(float yaw_deg, float pitch_deg)
{
    const float yaw   = Math::radians(yaw_deg);
    const float pitch = Math::radians(pitch_deg);
    return Math::Vec3f{Math::cos(yaw) * Math::cos(pitch), Math::sin(pitch), Math::sin(yaw) * Math::cos(pitch)};
}
} // namespace Utils

void FlyCamera::set_perspective(float fov_deg, float near_clip, float far_clip)
{
    m_fov  = fov_deg;
    m_near = near_clip;
    m_far  = far_clip;
}

void FlyCamera::set_aspect(float aspect)
{
    m_aspect = aspect;
}

void FlyCamera::update(float ts)
{
    if (!m_right_held)
    {
        return;
    }

    const Math::Vec3f forward  = Utils::compute_forward(m_yaw, m_pitch);
    const Math::Vec3f world_up = {0.0f, 1.0f, 0.0f};
    const Math::Vec3f right    = Math::cross(forward, world_up).normalized();
    const Math::Vec3f up       = Math::cross(right, forward).normalized();

    const float speed = m_move_speed * ts;

    if (Input::is_key_pressed(Key::W))
    {
        m_position += forward * speed;
    }
    if (Input::is_key_pressed(Key::S))
    {
        m_position -= forward * speed;
    }
    if (Input::is_key_pressed(Key::D))
    {
        m_position += right * speed;
    }
    if (Input::is_key_pressed(Key::A))
    {
        m_position -= right * speed;
    }
    if (Input::is_key_pressed(Key::E))
    {
        m_position += up * speed;
    }
    if (Input::is_key_pressed(Key::Q))
    {
        m_position -= up * speed;
    }
}

void FlyCamera::on_mouse_scroll(float delta)
{
    m_position += Utils::compute_forward(m_yaw, m_pitch) * (delta * m_zoom_speed);
}

void FlyCamera::on_mouse_button(MouseCode button, bool pressed)
{
    if (button == Mouse::ButtonRight)
    {
        m_right_held = pressed;
        if (pressed)
        {
            Input::get_mouse_position(m_last_mouse_x, m_last_mouse_y);
        }
    }
}

void FlyCamera::on_mouse_move(float x, float y)
{
    if (!m_right_held)
    {
        return;
    }

    const float dx = x - m_last_mouse_x;
    const float dy = y - m_last_mouse_y;
    m_last_mouse_x = x;
    m_last_mouse_y = y;

    m_yaw -= dx * m_orbit_speed;
    m_pitch -= dy * m_orbit_speed;
    m_pitch = Math::clamp(m_pitch, -89.0f, 89.0f);
}

void FlyCamera::focus_on(Math::Vec3f target, float distance)
{
    m_position = target - Utils::compute_forward(m_yaw, m_pitch) * distance;
}

void FlyCamera::focus_on(Math::Vec3f target, Math::Vec3f offset)
{
    const Math::Vec3f forward  = Utils::compute_forward(m_yaw, m_pitch);
    const Math::Vec3f world_up = {0.0f, 1.0f, 0.0f};
    const Math::Vec3f right    = Math::cross(forward, world_up).normalized();
    const Math::Vec3f up       = Math::cross(right, forward).normalized();

    Math::Vec3f total_offset = (right * offset.x) + (up * offset.y) - (forward * offset.z);
    m_position               = target + total_offset;

    Math::Vec3f new_dir = (target - m_position).normalized();

    m_yaw   = Math::degrees(Math::atan2(new_dir.z, new_dir.x));
    m_pitch = Math::degrees(Math::asin(new_dir.y));
    m_pitch = Math::clamp(m_pitch, -89.0f, 89.0f);
}

void FlyCamera::set_position(Math::Vec3f pos)
{
    m_position = pos;
}

void FlyCamera::set_orientation(float yaw_deg, float pitch_deg)
{
    m_yaw   = yaw_deg;
    m_pitch = Math::clamp(pitch_deg, -89.0f, 89.0f);
}

void FlyCamera::move(const Math::Vec3f& delta)
{
    m_position += delta;
}

void FlyCamera::rotate(float delta_yaw_deg, float delta_pitch_deg)
{
    m_yaw += delta_yaw_deg;
    m_pitch += delta_pitch_deg;

    m_pitch = Math::clamp(m_pitch, -89.0f, 89.0f);
}

CameraData FlyCamera::get_camera_data() const
{
    const Math::Vec3f forward = Utils::compute_forward(m_yaw, m_pitch);
    const Math::Vec3f world_up{0.0f, 1.0f, 0.0f};
    const Math::Mat4f view = Math::look_at(m_position, m_position + forward, world_up);
    const Math::Mat4f proj = Math::perspective(Math::radians(m_fov), m_aspect, m_near, m_far);
    return CameraData::from_matrices(view, proj, m_position);
}

} // namespace Ignis
