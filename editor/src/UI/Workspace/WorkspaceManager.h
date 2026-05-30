#pragma once

#ifdef ENGINE_IMGUI
#include <Ignis/UI/IImGuiDrawable.h>
#include "IWorkspace.h"
#include "../DockManager.h"
#include "../Commands/CommandDispatcher.h"

namespace Ignis
{

class WorkspaceManager : public IImGuiDrawable
{
public:
    using ThemeApplyFn = void (*)();

    explicit WorkspaceManager(PanelRegistry& registry);

    void register_workspace(UniquePtr<IWorkspace> ws);
    void register_theme(ThemeId id, ThemeApplyFn fn);

    // Deferred — transition executes at the top of the next draw_imgui() call.
    void activate(WorkspaceId id);

    // Dispatches to the active workspace's update(), which ticks all panels.
    void update(Timestep ts);

    // Returns the active workspace's data pointer (null if no active workspace).
    IWorkspaceData* active_data();

    // IImGuiDrawable
    void draw_imgui() override;
    bool has_pending_resize() override;
    void flush_resize() override;

    // Called by command execute() implementations.
    void cmd_open_panel(PanelId id);
    void cmd_close_panel(PanelId id);
    void cmd_reset_layout();
    void cmd_activate_workspace(WorkspaceId id);

    CommandDispatcher& dispatcher()
    {
        return m_dispatcher;
    }

private:
    void draw_workspace_tab_bar();
    void apply_theme(ThemeId id);
    void do_activate(WorkspaceId id);

    PanelRegistry&    m_registry;
    DockManager       m_dock;
    CommandDispatcher m_dispatcher;

    UnorderedMap<WorkspaceId, UniquePtr<IWorkspace>> m_workspaces;
    UnorderedMap<ThemeId, ThemeApplyFn>              m_themes;
    UnorderedMap<PanelId, bool>                      m_panel_open;

    WorkspaceId m_active_id    = 0;
    WorkspaceId m_pending_id   = 0;
    bool        m_layout_dirty = true;
};

} // namespace Ignis
#endif
