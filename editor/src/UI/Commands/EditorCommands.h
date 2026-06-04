#pragma once

#include "CommandDispatcher.h"
#include "../Panels/IPanel.h"
#include "../Workspace/WorkspaceDefinition.h"

namespace Ignis
{

struct OpenPanelCmd : IEditorCommand
{
    PanelId id;
    explicit OpenPanelCmd(PanelId p)
        : id(p)
    {
    }
    void execute(WorkspaceManager& mgr) override;
};

struct ClosePanelCmd : IEditorCommand
{
    PanelId id;
    explicit ClosePanelCmd(PanelId p)
        : id(p)
    {
    }
    void execute(WorkspaceManager& mgr) override;
};

struct ResetLayoutCmd : IEditorCommand
{
    void execute(WorkspaceManager& mgr) override;
};

struct ActivateWorkspaceCmd : IEditorCommand
{
    WorkspaceId target;
    explicit ActivateWorkspaceCmd(WorkspaceId w)
        : target(w)
    {
    }
    void execute(WorkspaceManager& mgr) override;
};

} // namespace Ignis
