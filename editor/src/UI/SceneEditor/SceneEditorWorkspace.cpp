#include "edpch.h"
#include "SceneEditorWorkspace.h"
#include "EditorSceneOverlay.h"

#include "../Commands/SceneCommands.h"
#include "../Panels/SceneTreePanel.h"
#include "../Panels/ConsolePanel.h"
#include "../Panels/ViewportPanel.h"
#include "../Panels/PropertyPanel.h"
#include "../Panels/AssetBrowserPanel.h"
#include "EditorPrimitives.h"
#include "Asset/EditorAssetManager.h"

#include <Ignis/Core/FileDialog.h>
#include "Project/ProjectManager.h"

#ifdef ENGINE_IMGUI
#endif

namespace Ignis
{

namespace
{

static constexpr InputMapping k_viewport_mappings[] = {
    {EditorActions::k_gizmo_translate, {Key::T, Modifier::Shift}},
    {EditorActions::k_gizmo_rotate, {Key::R, Modifier::Shift}},
    {EditorActions::k_gizmo_scale, {Key::S, Modifier::Shift}},
};

static constexpr PanelId     k_panel_scene_tree    = 1;
static constexpr PanelId     k_panel_viewport      = 2;
static constexpr PanelId     k_panel_console       = 3;
static constexpr PanelId     k_panel_properties    = 4;
static constexpr PanelId     k_panel_asset_browser = 5;
static constexpr WorkspaceId k_workspace_id        = 1;
} // namespace

namespace Utils
{
inline constexpr ImGuiShortcutString get_shortcut_name(ActionID id)
{
    return {InputSystem::get_action_label(id)};
}
} // namespace Utils

SceneEditorWorkspace::SceneEditorWorkspace(Scene& scene, SceneExtractor& extractor, SceneRenderer& sr,
                                           EditorSceneOverlay& overlay, PanelRegistry& registry,
                                           CommandDispatcher& dispatcher)
    : m_viewport_ctx("EditorViewport", k_viewport_mappings, std::size(k_viewport_mappings), 200)
{
    m_data.scene           = &scene;
    m_data.extractor       = &extractor;
    m_data.scene_renderer  = &sr;
    m_data.overlay         = &overlay;
    m_data.selected_entity = &m_selected_entity;
    m_data.sim_state       = &m_sim_state;
    m_data.gizmo           = &m_gizmo;
    m_data.dispatcher      = &dispatcher;
    m_data.builder         = &m_builder;

    m_def = make_definition(registry);

    m_panels.push_back(registry.create(k_panel_scene_tree));
    m_panels.push_back(registry.create(k_panel_viewport));
    m_panels.push_back(registry.create(k_panel_console));
    m_panels.push_back(registry.create(k_panel_properties));
    m_panels.push_back(registry.create(k_panel_asset_browser));

    InputSystem::register_context(&m_viewport_ctx);
    EditorInputManager::register_panel_context(&m_viewport_ctx, "Viewport");
}

SceneEditorWorkspace::~SceneEditorWorkspace()
{
    InputSystem::unregister_context(&m_viewport_ctx);
}

WorkspaceDefinition SceneEditorWorkspace::make_definition(PanelRegistry&)
{
    WorkspaceDefinition def;
    def.id                      = k_workspace_id;
    def.version                 = 1;
    def.name                    = "Scene Editor";
    def.allowed_panels          = {k_panel_scene_tree, k_panel_viewport, k_panel_console, k_panel_properties,
                                   k_panel_asset_browser};
    def.policies.allow_closing  = false;
    def.policies.allow_floating = false;
    def.theme_id                = 0;

    // root (split Left 0.20)
    //   child_a → leaf [SceneTree]
    //   child_b (split Right 0.25)
    //     child_a → leaf [Properties]
    //     child_b (split Down 0.25)
    //       child_a → leaf [Console]
    //       child_b → leaf [Viewport]

    auto center_bottom       = create_unique<LayoutNode>();
    center_bottom->split_dir = SplitDir::Down;
    center_bottom->ratio     = 0.25f;
    center_bottom->child_a   = create_unique<LayoutNode>();
    center_bottom->child_a->panel_ids.push_back(k_panel_console);
    center_bottom->child_a->panel_ids.push_back(k_panel_asset_browser);
    center_bottom->child_b = create_unique<LayoutNode>();
    center_bottom->child_b->panel_ids.push_back(k_panel_viewport);
    center_bottom->child_b->hide_tab_bar = true;

    auto right_split       = create_unique<LayoutNode>();
    right_split->split_dir = SplitDir::Right;
    right_split->ratio     = 0.25f;
    right_split->child_a   = create_unique<LayoutNode>();
    right_split->child_a->panel_ids.push_back(k_panel_properties);
    right_split->child_b = std::move(center_bottom);

    auto scene_tree_leaf = create_unique<LayoutNode>();
    scene_tree_leaf->panel_ids.push_back(k_panel_scene_tree);

    def.default_layout.split_dir = SplitDir::Left;
    def.default_layout.ratio     = 0.20f;
    def.default_layout.child_a   = std::move(scene_tree_leaf);
    def.default_layout.child_b   = std::move(right_split);

    return def;
}

void SceneEditorWorkspace::draw_menu_bar()
{
#ifdef ENGINE_IMGUI
    using namespace EditorActions;

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New Scene", Utils::get_shortcut_name(k_new_scene)))
        {
            m_data.dispatcher->enqueue(create_unique<NewSceneCmd>(m_data.scene, m_data.dispatcher));
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Load Project...", Utils::get_shortcut_name(k_load_project)))
        {
            ProjectManager&   pm = ProjectManager::get();
            FileDialogOptions opts;
            if (pm.is_open())
            {
                opts.initial_dir = pm.descriptor().root.parent_path();
            }
            if (auto path = FileDialog::open(FileDialogMode::OpenProject, opts))
            {
                Application::get().reset_frame_time();
                m_data.dispatcher->enqueue(create_unique<LoadProjectCmd>(*path, m_data.dispatcher));
            }
        }
        if (ImGui::MenuItem("Save Project", Utils::get_shortcut_name(k_save_project), false,
                            ProjectManager::get().is_open()))
        {
            m_data.dispatcher->enqueue(create_unique<SaveProjectCmd>());
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Save Scene", Utils::get_shortcut_name(k_save_scene)))
        {
            if (m_data.current_scene_path)
            {
                m_data.dispatcher->enqueue(create_unique<SaveSceneCmd>(m_data.scene, *m_data.current_scene_path));
            }
            else
            {
                FileDialogOptions opts;
                auto&             pm = ProjectManager::get();
                if (pm.is_open())
                {
                    opts.initial_dir = pm.descriptor().root / pm.descriptor().asset_source_dir;
                }
                if (auto path = FileDialog::save_scene(opts))
                {
                    Application::get().reset_frame_time();
                    m_data.current_scene_path = *path;
                    m_data.dispatcher->enqueue(create_unique<SaveSceneCmd>(m_data.scene, *path));
                }
            }
        }
        if (ImGui::MenuItem("Save Scene As..."))
        {
            FileDialogOptions opts;
            auto&             pm = ProjectManager::get();
            if (pm.is_open())
            {
                opts.initial_dir = pm.descriptor().root / pm.descriptor().asset_source_dir;
            }
            if (auto path = FileDialog::save_scene(opts))
            {
                Application::get().reset_frame_time();
                m_data.current_scene_path = *path;
                m_data.dispatcher->enqueue(create_unique<SaveSceneCmd>(m_data.scene, *path));
            }
        }
        if (ImGui::MenuItem("Load Scene", Utils::get_shortcut_name(k_load_scene)))
        {
            FileDialogOptions opts;
            ProjectManager&   pm = ProjectManager::get();
            if (pm.is_open())
            {
                opts.initial_dir = pm.descriptor().root / pm.descriptor().asset_source_dir;
            }
            if (auto path = FileDialog::open(FileDialogMode::OpenScene, opts))
            {
                Application::get().reset_frame_time();
                m_data.current_scene_path = *path;
                m_data.dispatcher->enqueue(create_unique<LoadSceneCmd>(m_data.scene, *path, m_data.dispatcher));
            }
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
        auto* d = m_data.dispatcher;
        if (ImGui::MenuItem("Undo", Utils::get_shortcut_name(k_undo), false, d && d->can_undo()))
        {
            d->undo();
        }
        if (ImGui::MenuItem("Redo", Utils::get_shortcut_name(k_redo), false, d && d->can_redo()))
        {
            d->redo();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Create"))
    {
        if (ImGui::MenuItem("Empty Entity"))
        {
            m_data.dispatcher->commit(create_unique<CreateEntityCmd>(m_data.scene, "Entity", m_data.selected_entity));
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Cube"))
        {
            EditorPrimitives::spawn_cube(*m_data.scene);
        }
        if (ImGui::MenuItem("Sphere"))
        {
            EditorPrimitives::spawn_sphere(*m_data.scene);
        }
        if (ImGui::MenuItem("Plane"))
        {
            EditorPrimitives::spawn_quad(*m_data.scene);
        }
        if (ImGui::MenuItem("Pyramid"))
        {
            EditorPrimitives::spawn_pyramid(*m_data.scene);
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Window"))
    {
        ImGui::MenuItem("Scene Tree", nullptr, nullptr);
        ImGui::MenuItem("Viewport", nullptr, nullptr);
        ImGui::MenuItem("Console", nullptr, nullptr);
        ImGui::MenuItem("Properties", nullptr, nullptr);
        ImGui::Separator();
        if (ImGui::MenuItem("Reset Layout"))
        {
        }
        ImGui::EndMenu();
    }
#endif
}

bool SceneEditorWorkspace::has_pending_resize() const
{
    for (const auto& p : m_panels)
    {
        if (p->has_pending_resize())
        {
            return true;
        }
    }
    return false;
}

void SceneEditorWorkspace::flush_resize()
{
    for (auto& p : m_panels)
    {
        if (p->has_pending_resize())
        {
            p->flush_resize(&m_data);
        }
    }
}

} // namespace Ignis
