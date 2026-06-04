#include "edpch.h"

#ifdef ENGINE_IMGUI
#include "WorkspaceManager.h"
#include "../Commands/EditorCommands.h"

namespace Ignis
{

// TODO: remove hardcoded colours and style values and replace with theme system integration!
constexpr int32_t MIN_WORKSPACES_FOR_TABS = 2;

WorkspaceManager::WorkspaceManager(PanelRegistry& registry, CommandDispatcher& dispatcher)
    : m_registry(registry),
      m_dock(registry),
      m_dispatcher(dispatcher)
{
}

void WorkspaceManager::register_workspace(UniquePtr<IWorkspace> ws)
{
    WorkspaceId id = ws->definition().id;
    m_workspaces.insert({id, std::move(ws)});
}

void WorkspaceManager::register_theme(ThemeId id, ThemeApplyFn fn)
{
    m_themes.insert({id, fn});
}

void WorkspaceManager::activate(WorkspaceId id)
{
    m_pending_id = id;
}

void WorkspaceManager::update(Timestep ts)
{
    auto it = m_workspaces.find(m_active_id);
    if (it != m_workspaces.end())
    {
        it->second->update(ts);
    }
}

IWorkspaceData* WorkspaceManager::active_data()
{
    auto it = m_workspaces.find(m_active_id);
    return it != m_workspaces.end() ? it->second->data() : nullptr;
}

void WorkspaceManager::draw_imgui()
{
    EditorInputManager::begin_frame();
    m_dispatcher.flush(*this);

    if (m_pending_id != m_active_id)
    {
        do_activate(m_pending_id);
    }

    auto ws_it = m_workspaces.find(m_active_id);
    if (ws_it == m_workspaces.end())
    {
        return;
    }

    IWorkspace&                active_ws  = *ws_it->second;
    const WorkspaceDefinition& active_def = active_ws.definition();

    const ImGuiViewport* vp        = ImGui::GetMainViewport();
    ImVec2               host_pos  = vp->Pos;
    ImVec2               host_size = vp->Size;

    const bool show_tabs = m_workspaces.size() > (MIN_WORKSPACES_FOR_TABS - 1);

    if (show_tabs)
    {
        float tab_bar_height = ImGui::GetFrameHeight();

        ImGui::SetNextWindowPos(host_pos);
        ImGui::SetNextWindowSize({host_size.x, tab_bar_height});
        ImGui::SetNextWindowViewport(vp->ID);

        ImGuiWindowFlags tab_host_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                          ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                          ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, {0.0f, 0.0f});

        ImVec4 theme_bg    = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        ImVec4 header_tint = ImVec4(theme_bg.x * 0.7f, theme_bg.y * 0.7f, theme_bg.z * 0.7f, theme_bg.w);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, header_tint);
        if (ImGui::Begin("##WorkspaceTabsHost", nullptr, tab_host_flags))
        {
            draw_workspace_tab_bar();
        }
        ImGui::End();

        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(4);

        host_pos.y += tab_bar_height;
        host_size.y -= tab_bar_height;
    }

    // Main Host Window Configuration
    ImGui::SetNextWindowPos(host_pos);
    ImGui::SetNextWindowSize(host_size);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                                  ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##WorkspaceHost", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    if (ImGui::BeginMenuBar())
    {
        active_ws.draw_menu_bar();
        ImGui::EndMenuBar();
    }

    ImGuiID ds_id =
        ImGui::DockSpace(ImGui::GetID("##MainDockspace"), {0.0f, 0.0f}, ImGuiDockNodeFlags_PassthruCentralNode);

    if (m_layout_dirty)
    {
        m_dock.compile_layout(ds_id, active_def.default_layout, host_size);
        m_layout_dirty = false;
    }

    // TODO: style the panel title bars! Also find a way to have them stand more out from the menu bar when they are
    // docked at the top, as currently they blend together a bit too much.
    for (const auto& panel : active_ws.get_active_panels())
    {
        auto it = m_panel_open.find(panel->get_id());
        if (it == m_panel_open.end() || !it->second)
        {
            continue;
        }

        bool  open   = true;
        bool* p_open = active_def.policies.allow_closing ? &open : nullptr;

        panel->push_window_style();
        ImGui::Begin(panel->get_title(), p_open, panel->get_window_flags());
        panel->draw(active_ws.data());
        EditorInputManager::set_panel_active(panel->get_title(),
                                             ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup));
        ImGui::End();
        panel->pop_window_style();

        if (p_open && !open)
        {
            m_dispatcher.enqueue(create_unique<ClosePanelCmd>(panel->get_id()));
        }
    }

    ImGui::End();
}

bool WorkspaceManager::has_pending_resize()
{
    auto it = m_workspaces.find(m_active_id);
    if (it == m_workspaces.end())
    {
        return false;
    }
    return it->second->has_pending_resize();
}

void WorkspaceManager::flush_resize()
{
    auto it = m_workspaces.find(m_active_id);
    if (it != m_workspaces.end())
    {
        it->second->flush_resize();
    }
}

void WorkspaceManager::cmd_open_panel(PanelId id)
{
    m_panel_open[id] = true;
}

void WorkspaceManager::cmd_close_panel(PanelId id)
{
    m_panel_open[id] = false;
}

void WorkspaceManager::cmd_reset_layout()
{
    m_layout_dirty = true;
}

void WorkspaceManager::cmd_activate_workspace(WorkspaceId id)
{
    m_pending_id = id;
}

void WorkspaceManager::draw_workspace_tab_bar()
{
    ImGui::PushStyleColor(ImGuiCol_TabSelectedOverline, ImVec4(0.38f, 0.52f, 0.67f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TabSelected, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_TabBorderSize, 0.0f);

    const ImGuiTabBarFlags tab_flags = ImGuiTabBarFlags_DrawSelectedOverline | ImGuiTabBarFlags_NoTooltip;

    if (ImGui::BeginTabBar("##WorkspaceTabs", tab_flags))
    {
        for (auto& [ws_id, ws] : m_workspaces)
        {
            if (ImGui::BeginTabItem(ws->definition().name))
            {
                if (ws_id != m_active_id)
                {
                    m_dispatcher.enqueue(create_unique<ActivateWorkspaceCmd>(ws_id));
                }
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
}

void WorkspaceManager::apply_theme(ThemeId id)
{
    auto it = m_themes.find(id);
    if (it != m_themes.end())
    {
        it->second();
    }
}

void WorkspaceManager::do_activate(WorkspaceId id)
{
    auto it = m_workspaces.find(id);
    if (it == m_workspaces.end())
    {
        return;
    }

    apply_theme(it->second->definition().theme_id);

    m_active_id    = id;
    m_layout_dirty = true;

    m_panel_open.clear();
    for (PanelId pid : it->second->definition().allowed_panels)
    {
        m_panel_open[pid] = true;
    }
}

void OpenPanelCmd::execute(WorkspaceManager& mgr)
{
    mgr.cmd_open_panel(id);
}
void ClosePanelCmd::execute(WorkspaceManager& mgr)
{
    mgr.cmd_close_panel(id);
}
void ResetLayoutCmd::execute(WorkspaceManager& mgr)
{
    mgr.cmd_reset_layout();
}
void ActivateWorkspaceCmd::execute(WorkspaceManager& mgr)
{
    mgr.cmd_activate_workspace(target);
}

} // namespace Ignis
#endif
