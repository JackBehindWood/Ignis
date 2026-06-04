#pragma once

#include "LayoutNode.h"

namespace Ignis
{

using WorkspaceId = uint32_t;
using ThemeId     = uint32_t;

struct DockPolicies
{
    bool allow_floating = false;
    // When true, each panel window is opened with ImGui::Begin's p_open
    // parameter. If ImGui sets it to false, WorkspaceManager enqueues
    // ClosePanelCmd rather than mutating panel state directly.
    bool allow_closing = true;
};

struct WorkspaceDefinition
{
    WorkspaceId     id;
    uint32_t        version = 1;
    const char*     name    = nullptr;
    Vector<PanelId> allowed_panels;
    LayoutNode      default_layout;
    DockPolicies    policies;
    ThemeId         theme_id = 0; // 0 = default; resolved in WorkspaceManager
};

} // namespace Ignis
