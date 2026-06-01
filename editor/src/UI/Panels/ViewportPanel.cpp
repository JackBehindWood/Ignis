#include "edpch.h"
#include "ViewportPanel.h"
#include "../Commands/SceneCommands.h"

#include "../SceneEditor/SceneEditorContext.h"
#include "../ImExt/ImExt.h"
#include <Ignis/Scene/SceneRenderer.h>
#include <Ignis/Scene/Entity.h>
#include <Ignis/Scene/Components/Components.h>
#include <Ignis/Rendering/Renderer.h>
#include <Ignis/Rendering/RenderMesh.h>
#include <imgui.h>

namespace Ignis
{

static constexpr PanelId k_id = 2;

namespace Utils
{

static Entity pick_entity(Scene& scene, const ImExt::WorldRay& ray)
{
    using namespace Ignis;
    auto   view   = scene.registry().view<TransformComponent, MeshRendererComponent>();
    float  best_t = FLT_MAX;
    Entity hit    = {};

    Math::Vec3f ray_dir = ray.dir;
    ray_dir.normalize();

    for (auto [handle, tc, mrc] : view.each())
    {
        const uint64_t        key  = static_cast<uint64_t>(mrc.mesh_id);
        SharedPtr<RenderMesh> mesh = Renderer::get_resource_cache().find_mesh(key);
        if (!mesh)
        {
            continue;
        }

        const Math::Vec3f world_center = tc.position;
        const float       max_scale =
            Math::max(Math::abs(tc.scale.x), Math::max(Math::abs(tc.scale.y), Math::abs(tc.scale.z)));
        const float world_radius = mesh->get_bounds_radius() * max_scale;

        const Math::Vec3f oc   = ray.origin - world_center;
        const float       b    = oc.dot(ray_dir);
        const float       c    = oc.dot(oc) - world_radius * world_radius;
        const float       disc = b * b - c;

        if (disc < 0.0f)
        {
            continue;
        }
        float disc_sqrt = Math::sqrt(disc);
        float t         = -b - disc_sqrt;
        if (t < 0.0f)
        {
            t = -b + disc_sqrt; // Try the back-side of the sphere
        }

        if (t > 0.0f && t < best_t)
        {
            best_t = t;
            hit    = Entity{handle, &scene};
        }
    }
    return hit;
}

static void draw_toolbar_overlay(SceneEditorData* data)
{
    using namespace Ignis;

    constexpr float k_padding = 10.0f;
    constexpr float k_btn_h   = 24.0f;
    constexpr float k_sep_w   = 10.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {4.0f, 0.0f});

    ImGui::PushStyleColor(ImGuiCol_Button, {0.08f, 0.08f, 0.08f, 0.72f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.22f, 0.22f, 0.22f, 0.88f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.30f, 0.30f, 0.30f, 1.00f});

    const ImVec4 k_accent = {0.38f, 0.52f, 0.67f, 1.00f};

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

} // namespace Utils

ViewportPanel::ViewportPanel()
{
    m_fly_camera.focus_on({0.0f, 0.0f, 0.0f}, {0.0f, 4.0f, -12.0f});
    m_fly_camera.set_perspective(60.0f, 0.1f, 1000.0f);
}

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

void ViewportPanel::update(float ts, IWorkspaceData* ctx)
{
    ImGuiIO&         io   = ImGui::GetIO();
    SceneEditorData* data = static_cast<SceneEditorData*>(ctx);

    using namespace EditorActions;
    if (data && data->gizmo)
    {
        if (InputSystem::was_action_started(k_gizmo_translate))
        {
            *data->gizmo = GizmoMode::Translate;
        }
        else if (InputSystem::was_action_started(k_gizmo_rotate))
        {
            *data->gizmo = GizmoMode::Rotate;
        }
        else if (InputSystem::was_action_started(k_gizmo_scale))
        {
            *data->gizmo = GizmoMode::Scale;
        }
    }

    // Snap: start new animation from an orient widget click
    if (m_orient_clicked >= 0)
    {
        static constexpr float k_snap_yaw[6]   = {180.0f, 0.0f, -90.0f, -90.0f, -90.0f, 90.0f};
        static constexpr float k_snap_pitch[6] = {0.0f, 0.0f, -89.0f, 89.0f, 0.0f, 0.0f};
        m_snap_yaw_start                       = m_fly_camera.get_yaw();
        m_snap_pitch_start                     = m_fly_camera.get_pitch();
        m_snap_yaw_target                      = k_snap_yaw[m_orient_clicked];
        m_snap_pitch_target                    = k_snap_pitch[m_orient_clicked];
        m_snap_t                               = 0.0f;
        m_orient_clicked                       = -1;
    }

    // RMB cancels active snap
    if (m_hovered && io.MouseClicked[1])
    {
        m_snap_t = 1.0f;
        m_fly_camera.on_mouse_button(Mouse::ButtonRight, true);
    }
    if (io.MouseReleased[1])
    {
        m_fly_camera.on_mouse_button(Mouse::ButtonRight, false);
    }
    if (m_hovered && io.MouseWheel != 0.0f)
    {
        m_fly_camera.on_mouse_scroll(io.MouseWheel);
    }

    m_fly_camera.on_mouse_move(io.MousePos.x, io.MousePos.y);

    const bool pressing_gizmo_shortcut = InputSystem::is_action_triggered(k_gizmo_translate) ||
                                         InputSystem::is_action_triggered(k_gizmo_rotate) ||
                                         InputSystem::is_action_triggered(k_gizmo_scale);
    if (!pressing_gizmo_shortcut)
    {
        m_fly_camera.update(ts);
    }

    // Apply smooth snap (smoothstep over ~0.2s)
    // TODO: we should probably create an Editor Animation System, for things like this!
    if (m_snap_t < 1.0f)
    {
        m_snap_t      = Math::min(m_snap_t + ts * 5.0f, 1.0f);
        const float s = m_snap_t * m_snap_t * (3.0f - 2.0f * m_snap_t);

        float dyaw = m_snap_yaw_target - m_snap_yaw_start;
        while (dyaw > 180.0f)
        {
            dyaw -= 360.0f;
        }
        while (dyaw < -180.0f)
        {
            dyaw += 360.0f;
        }

        m_fly_camera.set_orientation(m_snap_yaw_start + dyaw * s,
                                     m_snap_pitch_start + (m_snap_pitch_target - m_snap_pitch_start) * s);
    }

    if (data)
    {
        data->camera_data = m_fly_camera.get_camera_data();
    }
}

void ViewportPanel::draw(IWorkspaceData* ctx)
{
    SceneEditorData* data = static_cast<SceneEditorData*>(ctx);
    SceneRenderer&   sr   = *data->scene_renderer;

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

    m_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

    // Viewport bounds — valid after ImGui::Image above
    const ImVec2 vp_min  = ImGui::GetItemRectMin();
    const ImVec2 vp_sz   = {static_cast<float>(m_last_w ? m_last_w : aw), static_cast<float>(m_last_h ? m_last_h : ah)};
    const CameraData& cd = data->camera_data;

    bool gizmo_intercepted = false;

    if (data->selected_entity && data->selected_entity->is_valid())
    {
        ImExt::GizmoCtx gctx;
        gctx.vp_min    = vp_min;
        gctx.vp_size   = vp_sz;
        gctx.view_proj = cd.view_projection;
        gctx.view      = cd.view;
        gctx.proj      = cd.projection;
        gctx.cam_pos   = cd.position;

        TransformComponent& tc = data->selected_entity->get_component<TransformComponent>();
        Math::Transformf    xf{tc.position, tc.rotation, tc.scale};

        if (ImExt::ManipulateTransform(xf, *data->gizmo, gctx))
        {
            tc.position = xf.position;
            tc.rotation = xf.rotation;
            tc.scale    = xf.scale;
        }

        const bool gizmo_active = ImExt::IsGizmoActive();

        if (gizmo_active && !m_gizmo_was_active)
        {
            m_gizmo_before = {tc.position, tc.rotation, tc.scale};
        }

        if (!gizmo_active && m_gizmo_was_active && data->dispatcher)
        {
            data->dispatcher->commit(create_unique<ChangeTransformCmd>(
                *data->selected_entity, m_gizmo_before, Math::Transformf{tc.position, tc.rotation, tc.scale}));
        }

        m_gizmo_was_active = gizmo_active;
        gizmo_intercepted  = gizmo_active;
    }

    if (data->sim_state && data->gizmo)
    {
        Utils::draw_toolbar_overlay(data);
    }

    const ImVec2 orient_center{vp_min.x + vp_sz.x - 70.0f, vp_min.y + 70.0f};
    m_orient_clicked = ImExt::DrawCameraOrientation(cd.view, orient_center, 50.0f);

    ImVec2 mouse_pos          = ImGui::GetMousePos();
    float  dx                 = mouse_pos.x - orient_center.x;
    float  dy                 = mouse_pos.y - orient_center.y;
    bool   over_orient_widget = (dx * dx + dy * dy) <= (50.0f * 50.0f);

    if (m_hovered && ImGui::IsMouseClicked(0) && !gizmo_intercepted && !over_orient_widget)
    {
        ImVec2 win_pos      = ImGui::GetWindowPos();
        bool   over_toolbar = (mouse_pos.x >= win_pos.x && mouse_pos.x <= win_pos.x + 240.0f &&
                               mouse_pos.y >= win_pos.y && mouse_pos.y <= win_pos.y + 44.0f);

        if (!over_toolbar)
        {
            const ImExt::WorldRay ray        = ImExt::screen_to_world_ray(mouse_pos, vp_min, vp_sz, cd);
            Entity                hit_entity = Utils::pick_entity(*data->scene, ray);

            *data->selected_entity = hit_entity;
        }
    }
}

void ViewportPanel::flush_resize(IWorkspaceData* ctx)
{
    if (!m_resize_pending)
    {
        return;
    }

    SceneEditorData* data = static_cast<SceneEditorData*>(ctx);
    data->scene_renderer->resize(m_pending_w, m_pending_h);
    m_fly_camera.set_aspect(static_cast<float>(m_pending_w) / static_cast<float>(m_pending_h));
    m_last_w         = m_pending_w;
    m_last_h         = m_pending_h;
    m_resize_pending = false;
}

} // namespace Ignis
