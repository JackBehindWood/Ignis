#pragma once

#include <imgui.h>
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

// Returns true when the transform was mutated this frame.
bool ManipulateTransform(Math::Transformf& transform, GizmoMode mode, const GizmoCtx& ctx);

// Draws a small XYZ orientation widget at `center`. Returns clicked axis index
// (0=+X 1=-X 2=+Y 3=-Y 4=+Z 5=-Z) or -1 if nothing was clicked.
int DrawCameraOrientation(const Math::Mat4f& view, ImVec2 center, float size = 50.0f);

} // namespace ImExt
