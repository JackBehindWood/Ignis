#include "edpch.h"
#include "EditorLayer.h"
#include "EditorAssetManager.h"
#include "EditorSettingsManager.h"
#include "Project/ProjectManager.h"
#include "Ignis/Rendering/Renderer.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/MaterialFactory.h"
#include "EditorShaderCache.h"
#include "Ignis/Events/MouseEvent.h"

namespace Ignis
{
void EditorLayer::draw_grid(RGTextureHandle bb)
{
    m_builder.write_render_target(0, bb, RGColorAttachmentDesc::load());
    RGTextureHandle depth = m_builder.import_viewport_depth();
    m_builder.read_depth_stencil(depth, {GRILoadAction::Load, GRIStoreAction::DontCare, 1.0f});

    m_builder.add_pass("EditorGrid",
                       [this](GRICommandList& cmd)
                       {
                           Renderer::bind_frame_data(cmd);
                           cmd.set_graphics_pipeline_state(m_grid_material->get_pipeline_state());
                           cmd.draw_primitives(6);
                       });
}

EditorLayer::EditorLayer()
    : Layer("EditorLayer")
{
}

void EditorLayer::attach()
{
    IG_ASSERT(ProjectManager::get().is_open(), "EditorLayer requires an open project before attach");

    EditorAssetManager& assets = EditorAssetManager::get();

    m_scene_texture_id = assets.import_texture("test.png");
    assets.load_deferred(m_scene_texture_id);

    const AssetID mesh_id     = assets.import_mesh("triangle.obj");
    const AssetID material_id = assets.import_material("triangle.igmat");
    assets.load_deferred(mesh_id);
    assets.load_deferred(material_id);

    Entity             e     = m_active_scene.create_entity();
    TransformComponent trans = e.add_component<TransformComponent>();
    MeshComponent      mc;
    mc.mesh_id     = mesh_id;
    mc.material_id = material_id;
    e.add_component<MeshComponent>(mc);

    m_fly_camera.focus_on(trans.position, {0.0f, 3.0f, 3.0f});
    // m_fly_camera.focus_on(trans.position, 3.0f);

    GRIViewport* viewport = Application::get().get_window().get_viewport();
    const float  aspect   = (viewport && viewport->get_height() > 0)
                                ? static_cast<float>(viewport->get_width()) / static_cast<float>(viewport->get_height())
                                : 1.778f;
    m_fly_camera.set_aspect(aspect);

    m_grid_vs = EditorShaderCache::get().get_or_compile("grid.hlsl", GRIShaderStage::Vertex);
    m_grid_ps = EditorShaderCache::get().get_or_compile("grid.hlsl", GRIShaderStage::Pixel);

    const RendererConfig& cfg = Renderer::get_config();
    m_grid_material = Renderer::get_material_factory().get_or_create(m_grid_vs, m_grid_ps, "", cfg.render_target_format,
                                                                     cfg.depth_format, false, GRIBlendMode::AlphaBlend);

    m_scene_ready = true;
}

void EditorLayer::detach()
{
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

    m_fly_camera.update(ts);

    const CameraData camera = m_fly_camera.get_camera_data();

    Renderer::begin_frame(viewport);
    Renderer::upload_frame_data({camera.view_projection, camera.position});

    GRICommandList& cmd = RenderSystem::get_command_list();

    m_scene_renderer.prepare(m_active_scene);

    RGTextureHandle bb = m_builder.import_backbuffer();
    m_scene_renderer.render_scene(m_active_scene, camera, m_builder, bb, m_scene_texture_id);
    draw_grid(bb); // Note: when we draw the grid, for some reason when we move the camera to the positive y, the
                   // triangle seems to half in size!

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
    dispatcher.dispatch<WindowResizeEvent>(IG_BIND_EVENT_FN(EditorLayer::window_resized));
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

bool EditorLayer::window_resized(WindowResizeEvent& e)
{
    GRIViewport* viewport = e.get_viewport();
    const float  aspect   = (viewport && viewport->get_height() > 0)
                                ? static_cast<float>(viewport->get_width()) / static_cast<float>(viewport->get_height())
                                : 1.778f;
    m_fly_camera.set_aspect(aspect);

    return false;
}

} // namespace Ignis
