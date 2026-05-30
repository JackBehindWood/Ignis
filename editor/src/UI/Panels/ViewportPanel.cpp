#include "edpch.h"
#include "ViewportPanel.h"

#include "../SceneEditor/SceneEditorContext.h"
#include <Ignis/Scene/SceneRenderer.h>
#include <imgui.h>

namespace
{

static void draw_toolbar_overlay(Ignis::SceneEditorData* data)
{
    using namespace Ignis;

    constexpr float k_padding = 8.0f;
    constexpr float k_btn_h   = 22.0f;
    constexpr float k_sep_w   = 6.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {4.0f, 0.0f});

    ImGui::PushStyleColor(ImGuiCol_Button, {0.08f, 0.08f, 0.08f, 0.72f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.22f, 0.22f, 0.22f, 0.88f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.30f, 0.30f, 0.30f, 1.00f});

    const ImVec4 k_accent = {0.26f, 0.59f, 1.00f, 1.00f};

    ImGui::SetCursorPos({k_padding, k_padding});

    auto sim_btn = [&](const char* label, SimulationState state)
    {
        bool active = (*data->sim_state == state);
        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, k_accent);
        }
        if (ImGui::Button(label, {0.0f, k_btn_h}))
        {
            *data->sim_state = state;
        }
        if (active)
        {
            ImGui::PopStyleColor();
        }
        ImGui::SameLine();
    };

    sim_btn("Play", SimulationState::Playing);
    sim_btn("Pause", SimulationState::Paused);
    sim_btn("Stop", SimulationState::Stopped);

    ImGui::Dummy({k_sep_w, k_btn_h});
    ImGui::SameLine();

    auto gizmo_btn = [&](const char* label, GizmoMode mode)
    {
        bool active = (*data->gizmo == mode);
        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, k_accent);
        }
        if (ImGui::Button(label, {k_btn_h, k_btn_h}))
        {
            *data->gizmo = mode;
        }
        if (active)
        {
            ImGui::PopStyleColor();
        }
        ImGui::SameLine();
    };

    gizmo_btn("T", GizmoMode::Translate);
    gizmo_btn("R", GizmoMode::Rotate);
    gizmo_btn("S", GizmoMode::Scale);

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

} // namespace

namespace Ignis
{

static constexpr PanelId k_id = 2;

PanelId ViewportPanel::get_id() const
{
    return k_id;
}

ImGuiWindowFlags ViewportPanel::get_window_flags() const
{
    return ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
}

void ViewportPanel::push_window_style()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
}

void ViewportPanel::pop_window_style()
{
    ImGui::PopStyleVar();
}

void ViewportPanel::draw(IWorkspaceData* ctx)
{
    auto*          data = static_cast<SceneEditorData*>(ctx);
    SceneRenderer& sr   = *data->scene_renderer;

    ImVec2   avail = ImGui::GetContentRegionAvail();
    uint32_t aw    = static_cast<uint32_t>(avail.x);
    uint32_t ah    = static_cast<uint32_t>(avail.y);

    if (aw > 0 && ah > 0 && (aw != m_last_w || ah != m_last_h))
    {
        m_pending_w      = aw;
        m_pending_h      = ah;
        m_resize_pending = true;
    }

    GRITexture2D* color_rt = sr.get_color_rt();
    if (color_rt)
    {
        ImTextureID tex_id = reinterpret_cast<ImTextureID>(color_rt->get_native_handle());
        ImGui::Image(tex_id,
                     {static_cast<float>(m_last_w ? m_last_w : aw), static_cast<float>(m_last_h ? m_last_h : ah)});
    }

    if (data->sim_state && data->gizmo)
    {
        draw_toolbar_overlay(data);
    }
}

void ViewportPanel::flush_resize(IWorkspaceData* ctx)
{
    if (!m_resize_pending)
    {
        return;
    }

    auto* data = static_cast<SceneEditorData*>(ctx);
    data->scene_renderer->resize(m_pending_w, m_pending_h);
    m_last_w         = m_pending_w;
    m_last_h         = m_pending_h;
    m_resize_pending = false;
}

} // namespace Ignis
