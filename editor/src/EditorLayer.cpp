#include "edpch.h"
#include "EditorLayer.h"
#include "EditorAssetManager.h"
#include "EditorSettingsManager.h"
#include "Project/ProjectManager.h"
#include "Ignis/Rendering/Renderer.h"
#include <Ignis/Core/FileDialog.h>
#include "UI/Commands/SceneCommands.h"
#include "UI/SceneEditor/SceneEditorContext.h"

#ifdef ENGINE_IMGUI
#include <Ignis/UI/ImGuiLayer.h>
#include "UI/SceneEditor/SceneEditorWorkspace.h"
#include "UI/Panels/SceneTreePanel.h"
#include "UI/Panels/ConsolePanel.h"
#include "UI/Panels/ViewportPanel.h"
#include "UI/Panels/PropertyPanel.h"
#endif

namespace Ignis
{

namespace
{
constexpr InputMapping s_global_editor_mappings[] = {
    {EditorActions::k_undo, {Key::Z, Modifier::Super}},
    {EditorActions::k_redo, {Key::Y, Modifier::Super}},
    {EditorActions::k_reload_assets, {Key::F5, Modifier::None}},
    {EditorActions::k_new_scene, {Key::N, Modifier::Super}},
    {EditorActions::k_save_scene, {Key::S, Modifier::Super}},
    {EditorActions::k_save_project, {Key::S, Modifier::Super | Modifier::Shift}},
    {EditorActions::k_load_scene, {Key::O, Modifier::Super}},
    {EditorActions::k_load_project, {Key::O, Modifier::Super | Modifier::Shift}}};
}

EditorLayer::EditorLayer()
    : Layer("EditorLayer"),
      m_global_editor_ctx("GlobalEditor", s_global_editor_mappings, std::size(s_global_editor_mappings), 100)
{
}

void EditorLayer::attach()
{
    IG_ASSERT(ProjectManager::get().is_open(), "EditorLayer requires an open project before attach");

    ComponentInspector::register_defaults();

    GRIViewport*   viewport = Application::get().get_window().get_viewport();
    const uint32_t init_w   = viewport ? viewport->get_width() : 1280u;
    const uint32_t init_h   = viewport ? viewport->get_height() : 720u;
    m_scene_renderer.resize(init_w, init_h);

    m_scene_ready = true;

#ifdef ENGINE_IMGUI
    // TODO: this should probably be done per workspace!
    m_panel_registry.register_panel(
        {1, "Scene Tree", []() -> UniquePtr<IPanel> { return create_unique<SceneTreePanel>(); }});
    m_panel_registry.register_panel(
        {2, "Viewport", []() -> UniquePtr<IPanel> { return create_unique<ViewportPanel>(); }});
    m_panel_registry.register_panel(
        {3, "Console", []() -> UniquePtr<IPanel> { return create_unique<ConsolePanel>(); }});
    m_panel_registry.register_panel(
        {4, "Properties", []() -> UniquePtr<IPanel> { return create_unique<PropertyPanel>(); }});

    m_workspace_manager.register_workspace(
        create_unique<SceneEditorWorkspace>(m_active_scene, m_scene_renderer, m_panel_registry, m_dispatcher));

    m_workspace_manager.activate(1);

    ImGuiLayer::register_drawable(&m_workspace_manager);

    InputSystem::register_context(&m_global_editor_ctx);
    EditorInputManager::init();
#endif
}

void EditorLayer::detach()
{
#ifdef ENGINE_IMGUI
    ImGuiLayer::unregister_drawable(&m_workspace_manager);
    InputSystem::unregister_context(&m_global_editor_ctx);
    EditorInputManager::shutdown();
#endif
}

void EditorLayer::update(Timestep ts)
{
    InputSystem::begin_frame();

    EditorAssetManager::get().update(2.0f);

    if (!m_scene_ready)
    {
        return;
    }

#ifdef ENGINE_IMGUI
    using namespace EditorActions;
    if (InputSystem::was_action_started(k_undo))
    {
        m_dispatcher.undo();
    }
    if (InputSystem::was_action_started(k_redo))
    {
        m_dispatcher.redo();
    }
    if (InputSystem::was_action_started(k_reload_assets))
    {
        Renderer::get_resource_cache().clear();
        Renderer::clear_pipeline_cache();
        EditorAssetManager::get().reload_all();
        EditorResourceCache::get().reload();
    }

    auto* ws_data = static_cast<SceneEditorData*>(m_workspace_manager.active_data());

    if (InputSystem::was_action_started(k_new_scene))
    {
        m_dispatcher.enqueue(create_unique<NewSceneCmd>(&m_active_scene, &m_dispatcher));
    }
    if (InputSystem::was_action_started(k_load_scene))
    {
        FileDialogOptions opts;
        auto&             pm = ProjectManager::get();
        if (pm.is_open())
        {
            opts.initial_dir = pm.descriptor().root / pm.descriptor().asset_source_dir;
        }
        if (auto path = FileDialog::open(FileDialogMode::OpenScene, opts))
        {
            Application::get().reset_frame_time();
            if (ws_data)
            {
                ws_data->current_scene_path = *path;
            }
            m_dispatcher.enqueue(create_unique<LoadSceneCmd>(&m_active_scene, *path, &m_dispatcher));
        }
    }
    if (InputSystem::was_action_started(k_save_scene))
    {
        if (ws_data && ws_data->current_scene_path)
        {
            m_dispatcher.enqueue(create_unique<SaveSceneCmd>(&m_active_scene, *ws_data->current_scene_path));
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
                if (ws_data)
                {
                    ws_data->current_scene_path = *path;
                }
                m_dispatcher.enqueue(create_unique<SaveSceneCmd>(&m_active_scene, *path));
            }
        }
    }
    if (InputSystem::was_action_started(k_load_project))
    {
        FileDialogOptions opts;
        auto&             pm = ProjectManager::get();
        if (pm.is_open())
        {
            opts.initial_dir = pm.descriptor().root.parent_path();
        }
        if (auto path = FileDialog::open(FileDialogMode::OpenProject, opts))
        {
            Application::get().reset_frame_time();
            m_dispatcher.enqueue(create_unique<LoadProjectCmd>(*path, &m_dispatcher));
        }
    }
    if (InputSystem::was_action_started(k_save_project))
    {
        m_dispatcher.enqueue(create_unique<SaveProjectCmd>());
    }
#endif

    m_active_scene.update(ts);

#ifdef ENGINE_IMGUI
    m_workspace_manager.update(ts);
#endif

    m_scene_renderer.prepare(m_active_scene);
}

void EditorLayer::render()
{
}

void EditorLayer::event(Event& event)
{
    InputSystem::on_event(event);
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<WindowResizeEvent>(IG_BIND_EVENT_FN(EditorLayer::window_resized));
}

bool EditorLayer::window_resized(WindowResizeEvent&)
{
    return false;
}

} // namespace Ignis
