#include "edpch.h"
#include "SceneEditor.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Ignis
{

void SceneEditor::draw_imgui()
{
    IG_ASSERT(m_scene && m_scene_renderer, "SceneEditor not initialized");

    setup_dockspace_host_window();

    ImGuiID dockspace_id =
        ImGui::DockSpace(ImGui::GetID("##MainDockspace"), {0.0f, 0.0f}, ImGuiDockNodeFlags_PassthruCentralNode);

    if (m_first_run)
    {
        build_default_layout(dockspace_id);
        m_first_run = false;
    }

    draw_menu_bar();
    draw_toolbar();

    if (m_show_scene_tree)
    {
        m_scene_tree.draw(*m_scene, m_selected_entity);
    }
    if (m_show_console)
    {
        m_console.draw();
    }
    if (m_show_viewport)
    {
        m_viewport.draw(*m_scene_renderer);
    }
    if (m_show_properties)
    {
        m_properties.draw(*m_scene, m_selected_entity);
    }

    ImGui::End();
}

void SceneEditor::setup_dockspace_host_window()
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                             ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##DockspaceHost", nullptr, flags);
    ImGui::PopStyleVar(3);
}

void SceneEditor::build_default_layout(ImGuiID dockspace_id)
{
    ImGuiViewport* vp = ImGui::GetMainViewport();

    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, vp->Size);

    ImGuiID remaining = dockspace_id;
    ImGuiID dock_left, dock_right, dock_bottom;

    dock_left   = ImGui::DockBuilderSplitNode(remaining, ImGuiDir_Left, 0.20f, nullptr, &remaining);
    dock_right  = ImGui::DockBuilderSplitNode(remaining, ImGuiDir_Right, 0.25f, nullptr, &remaining);
    dock_bottom = ImGui::DockBuilderSplitNode(remaining, ImGuiDir_Down, 0.25f, nullptr, &remaining);

    ImGui::DockBuilderDockWindow("Scene Tree", dock_left);
    ImGui::DockBuilderDockWindow("Viewport", remaining);
    ImGui::DockBuilderDockWindow("Console", dock_bottom);
    ImGui::DockBuilderDockWindow("Properties", dock_right);

    ImGui::DockBuilderFinish(dockspace_id);
}

void SceneEditor::draw_menu_bar()
{
    if (!ImGui::BeginMenuBar())
    {
        return;
    }

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
        {
        }
        if (ImGui::MenuItem("Load Scene", "Ctrl+O"))
        {
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit"))
        {
            Application::get().close();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Window"))
    {
        ImGui::MenuItem("Scene Tree", nullptr, &m_show_scene_tree);
        ImGui::MenuItem("Viewport", nullptr, &m_show_viewport);
        ImGui::MenuItem("Console", nullptr, &m_show_console);
        ImGui::MenuItem("Properties", nullptr, &m_show_properties);
        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

void SceneEditor::draw_toolbar()
{
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 160.0f) * 0.5f);

    const ImVec4 active_col = {0.20f, 0.45f, 0.80f, 1.0f};
    const ImVec4 normal_col = ImGui::GetStyle().Colors[ImGuiCol_Button];

    auto sim_button = [&](const char* label, SimulationState state)
    {
        bool is_active = (m_sim_state == state);
        if (is_active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, active_col);
        }
        if (ImGui::SmallButton(label))
        {
            m_sim_state = state;
        }
        if (is_active)
        {
            ImGui::PopStyleColor();
        }
        ImGui::SameLine();
    };

    sim_button("Play", SimulationState::Playing);
    sim_button("Pause", SimulationState::Paused);
    sim_button("Stop", SimulationState::Stopped);

    ImGui::Separator();
    ImGui::SameLine();

    auto gizmo_button = [&](const char* label, GizmoMode mode)
    {
        bool is_active = (m_gizmo == mode);
        if (is_active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, active_col);
        }
        if (ImGui::SmallButton(label))
        {
            m_gizmo = mode;
        }
        if (is_active)
        {
            ImGui::PopStyleColor();
        }
        ImGui::SameLine();
    };

    gizmo_button("T", GizmoMode::Translate);
    gizmo_button("R", GizmoMode::Rotate);
    gizmo_button("S", GizmoMode::Scale);
}

} // namespace Ignis
