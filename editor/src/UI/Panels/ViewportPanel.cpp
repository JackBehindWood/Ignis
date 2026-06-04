#include "edpch.h"
#include "ViewportPanel.h"
#include "../Commands/SceneCommands.h"

#include "../SceneEditor/SceneEditorContext.h"
#include "../ImExt/ImExt.h"
#include "EditorResourceCache.h"
#include <Ignis/Scene/SceneRenderer.h>
#include <Ignis/Scene/Entity.h>
#include <Ignis/Scene/Components/Components.h>
#include <Ignis/Rendering/Renderer.h>
#include <Ignis/Rendering/RenderSystem.h>
#include <Ignis/Rendering/RenderMesh.h>
#include <Ignis/Rendering/RenderGraph/RGBuilder.h>
#include <Ignis/Core/Application.h>

namespace Ignis
{

static constexpr PanelId k_id = 2;

namespace
{

static void draw_grid(RGTextureHandle color, RGTextureHandle depth, RGBuilder& builder)
{
    Material* grid_mat = EditorResourceCache::get().get_material(EditorMaterial::Grid).get();
    if (!grid_mat)
    {
        return;
    }
    builder.write_render_target(0, color, RGColorAttachmentDesc::load());
    builder.read_depth_stencil(depth, {GRILoadAction::Load, GRIStoreAction::DontCare, 1.0f});
    builder.add_pass("EditorGrid",
                     [grid_mat](GRICommandList& cmd)
                     {
                         Renderer::bind_frame_data(cmd);
                         cmd.set_graphics_pipeline_state(grid_mat->get_pipeline_state());
                         cmd.draw_primitives(6);
                     });
}

static void draw_outline_composite(RGTextureHandle color_rt, RGTextureHandle mask_rt, SceneRenderer& sr,
                                   RGBuilder& builder)
{
    GRITexture2D* mask_tex    = sr.get_sel_mask_rt();
    Material*     outline_mat = EditorResourceCache::get().get_material(EditorMaterial::SelectionOutline).get();
    GRIBuffer*    params_buf  = EditorResourceCache::get().get_outline_params();
    if (!mask_tex || !outline_mat || !params_buf)
    {
        return;
    }
    builder.write_render_target(0, color_rt, RGColorAttachmentDesc::load());
    builder.read_texture(mask_rt);
    builder.add_pass("OutlineComposite",
                     [outline_mat, params_buf, mask_tex](GRICommandList& cmd)
                     {
                         cmd.set_graphics_pipeline_state(outline_mat->get_pipeline_state());
                         cmd.set_texture(mask_tex, 0, GRIShaderStage::Pixel);
                         cmd.set_uniform_buffer(params_buf, static_cast<uint32_t>(UniformSlot::MaterialArgs),
                                                GRIShaderStage::Pixel);
                         cmd.draw_primitives(3);
                     });
}

} // namespace

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
    auto& cache = EditorResourceCache::get();

    constexpr float k_padding   = 10.0f;
    constexpr float k_img_size  = 24.0f;
    constexpr float k_frame_pad = 2.0f;
    constexpr float k_btn_size  = k_img_size + 2.0f * k_frame_pad; // total button footprint: 28px
    constexpr float k_spacing   = 4.0f;

    const ImVec4 k_accent = {0.38f, 0.52f, 0.67f, 1.00f};

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {k_spacing, 0.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {k_frame_pad, k_frame_pad});

    ImGui::PushStyleColor(ImGuiCol_Button, {0.08f, 0.08f, 0.08f, 0.72f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.22f, 0.22f, 0.22f, 0.88f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.30f, 0.30f, 0.30f, 1.00f});

    const float window_width    = ImGui::GetWindowWidth();
    const float total_toolbar_w = 2.0f * k_btn_size + k_spacing;
    const float start_x         = (window_width - total_toolbar_w) * 0.5f;
    ImGui::SetCursorPos({start_x, k_padding});

    auto image_btn = [&](const char* id, GRITexture2D* icon, bool accent) -> bool
    {
        if (accent)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, k_accent);
        }
        bool clicked = false;
        if (icon)
        {
            ImTextureID tex_id = reinterpret_cast<ImTextureID>(icon->get_native_handle());
            clicked            = ImGui::ImageButton(id, tex_id, {k_img_size, k_img_size});
        }
        else
        {
            clicked = ImGui::Button(id, {k_btn_size, k_btn_size});
        }
        if (accent)
        {
            ImGui::PopStyleColor();
        }
        return clicked;
    };

    SimulationState sim   = *data->sim_state;
    GizmoMode&      gizmo = *data->gizmo;

    auto draw_gizmo_btn = [&]()
    {
        if (image_btn("##gizmo", cache.get_gizmo_icon(), false))
        {
            gizmo = static_cast<GizmoMode>((static_cast<uint8_t>(gizmo) + 1) % 3);
        }

        // Badge: single char at bottom-right corner of the button
        {
            const char* badge   = (gizmo == GizmoMode::Translate) ? "T" : (gizmo == GizmoMode::Rotate) ? "R" : "S";
            ImVec2      bmax    = ImGui::GetItemRectMax();
            ImVec2      txt_pos = {bmax.x - 9.0f, bmax.y - 12.0f};
            ImGui::GetWindowDrawList()->AddText(txt_pos, IM_COL32(255, 255, 255, 220), badge);
        }

        if (ImGui::IsItemHovered())
        {
            const char* tip = (gizmo == GizmoMode::Translate) ? "Gizmo Mode: Translate (T)"
                              : (gizmo == GizmoMode::Rotate)  ? "Gizmo Mode: Rotate (R)"
                                                              : "Gizmo Mode: Scale (S)";
            ImGui::SetTooltip("%s", tip);
        }
    };

    if (sim == SimulationState::Stopped)
    {
        if (image_btn("##play", cache.get_play_icon(), false))
        {
            *data->sim_state = SimulationState::Playing;
        }
        ImGui::SameLine();
        draw_gizmo_btn();
    }
    else if (sim == SimulationState::Playing)
    {
        if (image_btn("##pause", cache.get_pause_icon(), false))
        {
            *data->sim_state = SimulationState::Paused;
        }
        ImGui::SameLine();
        if (image_btn("##stop", cache.get_stop_icon(), false))
        {
            *data->sim_state = SimulationState::Stopped;
        }
    }
    else // Paused
    {
        if (image_btn("##play", cache.get_play_icon(), false))
        {
            *data->sim_state = SimulationState::Playing;
        }
        ImGui::SameLine();
        if (image_btn("##stop", cache.get_stop_icon(), false))
        {
            *data->sim_state = SimulationState::Stopped;
        }
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
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
    SceneEditorData* data    = static_cast<SceneEditorData*>(ctx);
    SceneRenderer&   sr      = *data->scene_renderer;
    RGBuilder&       builder = *data->builder;

    ImVec2   avail = ImGui::GetContentRegionAvail();
    uint32_t aw    = static_cast<uint32_t>(avail.x);
    uint32_t ah    = static_cast<uint32_t>(avail.y);

    if (aw > 0 && ah > 0 && (aw != m_last_w || ah != m_last_h))
    {
        m_pending_w      = aw;
        m_pending_h      = ah;
        m_resize_pending = true;
    }

    GRIViewport* viewport = Application::get().get_window().get_viewport();

    Renderer::begin_frame(viewport);
    Renderer::upload_frame_data({data->camera_data.view_projection, data->camera_data.position});

    GRICommandList& cmd = RenderSystem::get_command_list();

    auto [color_handle, depth_handle] = sr.render_scene(*data->scene, data->camera_data, builder);
    draw_grid(color_handle, depth_handle, builder);

    if (data->selected_entity && data->selected_entity->is_valid())
    {
        RGTextureHandle mask_rt = sr.draw_selection_mask(*data->selected_entity, depth_handle, builder);
        if (mask_rt.is_valid())
        {
            draw_outline_composite(color_handle, mask_rt, sr, builder);
        }
    }

    builder.execute(cmd);
    Renderer::end_frame();

    GRITexture2D* color_rt = sr.get_color_rt();
    if (color_rt)
    {
        ImTextureID tex_id = reinterpret_cast<ImTextureID>(color_rt->get_native_handle());
        ImGui::Image(tex_id,
                     {static_cast<float>(m_last_w ? m_last_w : aw), static_cast<float>(m_last_h ? m_last_h : ah)});
    }

    if (auto target = ImExt::DragDrop::Target<AssetDragPayload>())
    {
        if (const auto* hover = target.peek())
        {
            bool has_mesh = false;
            bool has_tex  = false;
            bool has_mat  = false;
            for (uint32_t i = 0; i < hover->count; ++i)
            {
                const char* tl = hover->items[i].type_label;
                if (std::strcmp(tl, "MSH") == 0)
                {
                    has_mesh = true;
                }
                else if (std::strcmp(tl, "TEX") == 0)
                {
                    has_tex = true;
                }
                else if (std::strcmp(tl, "MAT") == 0)
                {
                    has_mat = true;
                }
            }
            const bool compat = has_mesh || has_tex || has_mat;
            ImVec2     rmin   = ImGui::GetItemRectMin();
            ImVec2     rmax   = ImGui::GetItemRectMax();
            ImU32      tint   = compat ? IM_COL32(100, 200, 100, 30) : IM_COL32(200, 80, 80, 30);
            ImGui::GetWindowDrawList()->AddRectFilled(rmin, rmax, tint);
            target.draw_compat_feedback(compat, rmin, rmax, 0.f);
            if (has_mesh)
            {
                ImGui::SetTooltip("Drop to spawn mesh");
            }
            else if (has_tex || has_mat)
            {
                ImGui::SetTooltip("Drop to assign to selected entity");
            }
        }
        if (const AssetDragPayload* p = target.accept())
        {
            auto& pm = ProjectManager::get();
            if (pm.is_open() && data->dispatcher)
            {
                for (uint32_t i = 0; i < p->count; ++i)
                {
                    const auto& item = p->items[i];
                    Path        abs  = pm.descriptor().root / pm.descriptor().asset_source_dir / item.rel_path;

                    if (std::strcmp(item.type_label, "MSH") == 0)
                    {
                        String name = Path(item.rel_path).stem().string();
                        data->dispatcher->commit(create_unique<SpawnMeshFromAssetCmd>(data->scene, abs, std::move(name),
                                                                                      data->selected_entity));
                    }
                    else if (std::strcmp(item.type_label, "TEX") == 0 && data->selected_entity &&
                             data->selected_entity->is_valid())
                    {
                        AssetID tex_id = AssetManager::get().import(abs, AssetType::Texture2D);
                        Entity& ent    = *data->selected_entity;
                        if (!ent.has_component<TextureComponent>())
                        {
                            ent.add_component<TextureComponent>();
                        }
                        AssetID before = ent.get_component<TextureComponent>().texture_id;
                        data->dispatcher->commit(create_unique<AssignTextureToEntityCmd>(ent, before, tex_id));
                    }
                    else if (std::strcmp(item.type_label, "MAT") == 0 && data->selected_entity &&
                             data->selected_entity->is_valid())
                    {
                        AssetID mat_id = AssetManager::get().import(abs, AssetType::Material);
                        Entity& ent    = *data->selected_entity;
                        if (!ent.has_component<MaterialComponent>())
                        {
                            ent.add_component<MaterialComponent>();
                        }
                        AssetID before = ent.get_component<MaterialComponent>().material_id;
                        data->dispatcher->commit(create_unique<AssignMaterialToEntityCmd>(ent, before, mat_id));
                    }
                }
            }
        }
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
        ImVec2      win_pos      = ImGui::GetWindowPos();
        const float tb_half_w    = (2.0f * 28.0f + 4.0f) * 0.5f + 8.0f; // half total width + slop
        const float tb_cx        = win_pos.x + ImGui::GetWindowWidth() * 0.5f;
        bool        over_toolbar = (mouse_pos.x >= tb_cx - tb_half_w && mouse_pos.x <= tb_cx + tb_half_w &&
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
