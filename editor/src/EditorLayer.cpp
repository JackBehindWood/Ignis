#include "edpch.h"
#include "EditorLayer.h"
#include "EditorAssetManager.h"
#include "EditorSettingsManager.h"
#include "Project/ProjectManager.h"
#include "Ignis/Rendering/Renderer.h"
#include "Ignis/Rendering/RenderSystem.h"

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

    GRIViewport* viewport = Application::get().get_window().get_viewport();
    Renderer::begin_frame(viewport);

    GRICommandList& cmd = RenderSystem::get_command_list();

    RGTextureHandle bb = m_builder.import_backbuffer();
    m_scene_renderer.render_scene(m_active_scene, m_builder, bb, m_scene_texture_id);

    m_builder.execute(cmd);
    Renderer::end_frame();
}

void EditorLayer::event(Event& event)
{
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<KeyPressedEvent>(IG_BIND_EVENT_FN(EditorLayer::key_pressed));
}

bool EditorLayer::key_pressed(KeyPressedEvent& e)
{
    if (e.get_key_code() == Key::F5)
    {
        Renderer::get_resource_cache().clear();
        Renderer::clear_pipeline_cache();
        EditorAssetManager::get().reload_all();
        return true;
    }
    return false;
}

} // namespace Ignis
