#pragma once
#include <Ignis/Input/InputTypes.h>

namespace Ignis::EditorActions
{

constexpr ActionID k_undo            = action_id("Editor::Undo");
constexpr ActionID k_redo            = action_id("Editor::Redo");
constexpr ActionID k_reload_assets   = action_id("Editor::ReloadAssets");
constexpr ActionID k_gizmo_translate = action_id("Editor::GizmoTranslate");
constexpr ActionID k_gizmo_rotate    = action_id("Editor::GizmoRotate");
constexpr ActionID k_gizmo_scale     = action_id("Editor::GizmoScale");
constexpr ActionID k_save_scene      = action_id("Editor::SaveScene");
constexpr ActionID k_load_scene      = action_id("Editor::LoadScene");

} // namespace Ignis::EditorActions
