#pragma once

#include "../Panels/IWorkspaceData.h"
#include "../Commands/CommandDispatcher.h"
#include <Ignis/Scene/Scene.h>
#include <Ignis/Scene/Entity.h>
#include <Ignis/Scene/SceneExtractor.h>
#include <Ignis/Scene/SceneRenderer.h>
#include "EditorSceneOverlay.h"
#include <Ignis/Scene/CameraData.h>
#include <Ignis/Rendering/RenderGraph/RGBuilder.h>

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
    Scene*                scene           = nullptr;
    SceneExtractor*       extractor       = nullptr;
    SceneRenderer*        scene_renderer  = nullptr;
    EditorSceneOverlay*   overlay         = nullptr;
    Entity*               selected_entity = nullptr;
    SimulationState*      sim_state       = nullptr;
    GizmoMode*            gizmo           = nullptr;
    CameraData            camera_data     = CameraData::identity();
    CommandDispatcher*    dispatcher      = nullptr;
    RGBuilder*            builder         = nullptr;
    Optional<Path>        current_scene_path;
    Optional<Math::Vec3f> focus_request;
    UUID                  material_editor_target = UUID(UUID::s_invalid);
};

} // namespace Ignis
