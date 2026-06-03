#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <Ignis/Math/Mat4.h>
#include <Ignis/Math/Transform.h>
#include "UI/SceneEditor/SceneEditorContext.h"

namespace ImExt
{

using namespace Ignis;

struct GizmoCtx
{
    ImVec2      vp_min;  // screen-space top-left of the rendered scene image
    ImVec2      vp_size; // viewport pixel dimensions
    Math::Mat4f view_proj;
    Math::Mat4f view;
    Math::Mat4f proj;
    Math::Vec3f cam_pos;
};

struct WorldRay
{
    Math::Vec3f origin;
    Math::Vec3f dir;
};

// Returns true when the transform was mutated this frame.
bool ManipulateTransform(Math::Transformf& transform, GizmoMode mode, const GizmoCtx& ctx);

// Draws a small XYZ orientation widget at `center`. Returns clicked axis index
// (0=+X 1=-X 2=+Y 3=-Y 4=+Z 5=-Z) or -1 if nothing was clicked.
int DrawCameraOrientation(const Math::Mat4f& view, ImVec2 center, float size = 50.0f);

// Constructs a world-space ray from a screen-space pixel, viewport bounds, and camera matrices.
WorldRay screen_to_world_ray(ImVec2 screen_px, ImVec2 vp_min, ImVec2 vp_size, const CameraData& cam);

// True when a gizmo handle is actively being dragged this frame.
bool IsGizmoActive();

namespace DragDrop
{

inline bool begin_source(ImGuiDragDropFlags flags = 0)
{
    return ImGui::BeginDragDropSource(flags);
}

template <typename T>
void set_payload(const T& data)
{
    ImGui::SetDragDropPayload(T::k_type, &data, sizeof(T));
}

inline void end_source()
{
    ImGui::EndDragDropSource();
}

inline bool begin_target()
{
    return ImGui::BeginDragDropTarget();
}

template <typename T>
const T* accept(ImGuiDragDropFlags flags = 0)
{
    const ImGuiPayload* p = ImGui::AcceptDragDropPayload(T::k_type, flags);
    return p ? static_cast<const T*>(p->Data) : nullptr;
}

template <typename T>
const T* peek()
{
    return accept<T>(ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
}

inline void end_target()
{
    ImGui::EndDragDropTarget();
}

// Makes the entire current child window a drop target.
// Guard with !ImGui::IsAnyItemHovered() to yield priority to item-level targets.
inline bool begin_window_target()
{
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    return ImGui::BeginDragDropTargetCustom(w->Rect(), w->ID);
}

} // namespace DragDrop

} // namespace ImExt
