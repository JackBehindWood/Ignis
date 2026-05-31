#pragma once

#include "../Panels/IWorkspaceData.h"
#include "../Commands/CommandDispatcher.h"
#include <Ignis/Scene/Scene.h>
#include <Ignis/Scene/Entity.h>
#include <Ignis/Scene/SceneRenderer.h>
#include <Ignis/Scene/CameraData.h>

namespace Ignis
{

enum class SimulationState : uint8_t
{
    Stopped,
    Playing,
    Paused
};
enum class GizmoMode : uint8_t
{
    Translate,
    Rotate,
    Scale
};

struct SceneEditorData : IWorkspaceData
{
    Scene*             scene           = nullptr;
    SceneRenderer*     scene_renderer  = nullptr;
    Entity*            selected_entity = nullptr;
    SimulationState*   sim_state       = nullptr;
    GizmoMode*         gizmo           = nullptr;
    CameraData         camera_data     = CameraData::identity();
    CommandDispatcher* dispatcher      = nullptr;
};

} // namespace Ignis
