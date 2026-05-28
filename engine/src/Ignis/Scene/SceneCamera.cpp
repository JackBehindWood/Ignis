#include "igpch.h"
#include "Ignis/Scene/SceneCamera.h"
#include "yaml-cpp/yaml.h"

namespace Ignis
{

namespace
{
// Left-handed orthographic projection, depth range [0,1]
Math::Mat4f ortho(float left, float right, float bottom, float top, float near_z, float far_z)
{
    const float rl = right - left;
    const float tb = top - bottom;
    const float fn = far_z - near_z;

    Math::Mat4f r{};
    r.m[0]  = 2.0f / rl;
    r.m[5]  = 2.0f / tb;
    r.m[10] = 1.0f / fn;
    r.m[12] = -(right + left) / rl;
    r.m[13] = -(top + bottom) / tb;
    r.m[14] = -near_z / fn;
    r.m[15] = 1.0f;
    return r;
}
} // namespace

void SceneCamera::set_perspective(float fov_deg, float near_clip, float far_clip)
{
    m_type  = ProjectionType::Perspective;
    m_fov   = fov_deg;
    m_near  = near_clip;
    m_far   = far_clip;
    m_dirty = true;
}

void SceneCamera::set_orthographic(float ortho_size, float near_clip, float far_clip)
{
    m_type       = ProjectionType::Orthographic;
    m_ortho_size = ortho_size;
    m_near       = near_clip;
    m_far        = far_clip;
    m_dirty      = true;
}

void SceneCamera::set_aspect(float aspect)
{
    m_aspect = aspect;
    m_dirty  = true;
}

Math::Mat4f SceneCamera::get_projection() const
{
    if (!m_dirty)
    {
        return m_cached_proj;
    }

    if (m_type == ProjectionType::Perspective)
    {
        m_cached_proj = Math::perspective(Math::radians(m_fov), m_aspect, m_near, m_far);
    }
    else
    {
        const float half = m_ortho_size * 0.5f;
        m_cached_proj    = ortho(-half * m_aspect, half * m_aspect, -half, half, m_near, m_far);
    }
    m_dirty = false;
    return m_cached_proj;
}

YAML::Node serialize_scene_camera(const SceneCamera& cam)
{
    YAML::Node node;
    node["projection"] =
        (cam.projection_type() == SceneCamera::ProjectionType::Perspective) ? "Perspective" : "Orthographic";
    node["fov"]        = cam.fov();
    node["near_clip"]  = cam.near_clip();
    node["far_clip"]   = cam.far_clip();
    node["ortho_size"] = cam.ortho_size();
    return node;
}

SceneCamera deserialize_scene_camera(const YAML::Node& node)
{
    SceneCamera cam;
    if (!node)
    {
        return cam;
    }

    const std::string type      = node["projection"] ? node["projection"].as<std::string>() : "Perspective";
    const float       near_clip = node["near_clip"] ? node["near_clip"].as<float>() : 0.1f;
    const float       far_clip  = node["far_clip"] ? node["far_clip"].as<float>() : 1000.0f;

    if (type == "Orthographic")
    {
        const float ortho_size = node["ortho_size"] ? node["ortho_size"].as<float>() : 10.0f;
        cam.set_orthographic(ortho_size, near_clip, far_clip);
    }
    else
    {
        const float fov = node["fov"] ? node["fov"].as<float>() : 60.0f;
        cam.set_perspective(fov, near_clip, far_clip);
    }
    return cam;
}

} // namespace Ignis
