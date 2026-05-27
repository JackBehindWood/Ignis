#pragma once

#include "Ignis/Math/Math.h"

namespace YAML
{
class Emitter;
class Node;
} // namespace YAML

namespace Ignis
{

class SceneCamera
{
public:
    enum class ProjectionType : uint8_t
    {
        Perspective,
        Orthographic
    };

    void set_perspective(float fov_deg, float near_clip, float far_clip);
    void set_orthographic(float ortho_size, float near_clip, float far_clip);
    void set_aspect(float aspect);

    Math::Mat4f    get_projection() const;
    ProjectionType projection_type() const
    {
        return m_type;
    }
    float fov() const
    {
        return m_fov;
    }
    float near_clip() const
    {
        return m_near;
    }
    float far_clip() const
    {
        return m_far;
    }
    float ortho_size() const
    {
        return m_ortho_size;
    }
    float aspect() const
    {
        return m_aspect;
    }

private:
    ProjectionType      m_type       = ProjectionType::Perspective;
    float               m_fov        = 60.0f;
    float               m_near       = 0.1f;
    float               m_far        = 1000.0f;
    float               m_ortho_size = 10.0f;
    float               m_aspect     = 1.778f;
    mutable bool        m_dirty      = true;
    mutable Math::Mat4f m_cached_proj;
};

YAML::Node  serialize_scene_camera(const SceneCamera& cam);
SceneCamera deserialize_scene_camera(const YAML::Node& node);

} // namespace Ignis
