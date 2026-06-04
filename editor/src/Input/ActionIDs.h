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
constexpr ActionID k_load_project    = action_id("Editor::LoadProject");
constexpr ActionID k_save_project    = action_id("Editor::SaveProject");
constexpr ActionID k_new_scene       = action_id("Editor::NewScene");
constexpr ActionID k_save_scene      = action_id("Editor::SaveScene");
constexpr ActionID k_load_scene      = action_id("Editor::LoadScene");

} // namespace Ignis::EditorActions

namespace Ignis::AssetBrowserActions
{

constexpr ActionID k_nav_left        = action_id("AssetBrowser::NavLeft");
constexpr ActionID k_nav_right       = action_id("AssetBrowser::NavRight");
constexpr ActionID k_nav_up          = action_id("AssetBrowser::NavUp");
constexpr ActionID k_nav_down        = action_id("AssetBrowser::NavDown");
constexpr ActionID k_activate        = action_id("AssetBrowser::Activate");
constexpr ActionID k_clipboard_copy  = action_id("AssetBrowser::ClipboardCopy");
constexpr ActionID k_clipboard_cut   = action_id("AssetBrowser::ClipboardCut");
constexpr ActionID k_clipboard_paste = action_id("AssetBrowser::ClipboardPaste");
constexpr ActionID k_delete          = action_id("AssetBrowser::Delete");

} // namespace Ignis::AssetBrowserActions
