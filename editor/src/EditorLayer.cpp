#include "edpch.h"
#include "EditorLayer.h"
#include "EditorAssetManager.h"
#include "EditorSettingsManager.h"
#include "Project/ProjectManager.h"
#include "Ignis/Rendering/Renderer.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Events/MouseEvent.h"

namespace Ignis
{

EditorLayer::EditorLayer()
    : Layer("EditorLayer")
{
}

void EditorLayer::attach()
{
    IG_ASSERT(ProjectManager::get().is_open(), "EditorLayer requires an open project before attach");
    IG_INFO("EditorLayer attached");

    EditorAssetManager& assets = EditorAssetManager::get();

    m_scene_texture_id = assets.import_texture("test.png");
    assets.load_deferred(m_scene_texture_id);

    const AssetID mesh_id     = assets.import_mesh("triangle.obj");
    const AssetID material_id = assets.import_material("triangle.igmat");
    assets.load_deferred(mesh_id);
    assets.load_deferred(material_id);

    Entity e = m_active_scene.create_entity();
    e.add_component<TransformComponent>();
    MeshComponent mc;
    mc.mesh_id     = mesh_id;
    mc.material_id = material_id;
    e.add_component<MeshComponent>(mc);

    m_scene_ready = true;
}

void EditorLayer::detach()
{
    IG_INFO("EditorLayer detached");
}

void EditorLayer::update(Timestep ts)
{
    EditorAssetManager::get().update(2.0f);

    if (!m_scene_ready)
    {
        return;
    }

    m_active_scene.update(ts);

    GRIViewport* viewport = Application::get().get_window().get_viewport();

    const float aspect = (viewport && viewport->get_height() > 0)
                             ? static_cast<float>(viewport->get_width()) / static_cast<float>(viewport->get_height())
                             : 1.778f;
    m_fly_camera.set_aspect(aspect);
    m_fly_camera.update(ts);

    const CameraData camera = m_fly_camera.get_camera_data();

    Renderer::begin_frame(viewport);

    GRICommandList& cmd = RenderSystem::get_command_list();

    RGTextureHandle bb = m_builder.import_backbuffer();
    m_scene_renderer.render_scene(m_active_scene, camera, m_builder, bb, m_scene_texture_id);

    m_builder.execute(cmd);
    Renderer::end_frame();
}

void EditorLayer::event(Event& event)
{
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<KeyPressedEvent>(IG_BIND_EVENT_FN(EditorLayer::key_pressed));
    dispatcher.dispatch<MouseButtonPressedEvent>(IG_BIND_EVENT_FN(EditorLayer::mouse_button_pressed));
    dispatcher.dispatch<MouseButtonReleasedEvent>(IG_BIND_EVENT_FN(EditorLayer::mouse_button_released));
    dispatcher.dispatch<MouseMovedEvent>(IG_BIND_EVENT_FN(EditorLayer::mouse_moved));
    dispatcher.dispatch<MouseScrolledEvent>(IG_BIND_EVENT_FN(EditorLayer::mouse_scrolled));
}

bool EditorLayer::key_pressed(KeyPressedEvent& e)
{
    // TODO: add wasd keys for the fly camera!
    if (e.get_key_code() == Key::F5)
    {
        Renderer::get_resource_cache().clear();
        Renderer::clear_pipeline_cache();
        EditorAssetManager::get().reload_all();
        return true;
    }
    return false;
}

bool EditorLayer::mouse_button_pressed(MouseButtonPressedEvent& e)
{
    m_fly_camera.on_mouse_button(e.get_mouse_button(), true);
    return false;
}

bool EditorLayer::mouse_button_released(MouseButtonReleasedEvent& e)
{
    m_fly_camera.on_mouse_button(e.get_mouse_button(), false);
    return false;
}

bool EditorLayer::mouse_moved(MouseMovedEvent& e)
{
    m_fly_camera.on_mouse_move(e.get_x(), e.get_y());
    return false;
}

bool EditorLayer::mouse_scrolled(MouseScrolledEvent& e)
{
    m_fly_camera.on_mouse_scroll(e.get_y_offset());
    return false;
}

} // namespace Ignis
