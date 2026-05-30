#include "edpch.h"
#include "EditorLayer.h"
#include "EditorAssetManager.h"
#include "EditorSettingsManager.h"
#include "Project/ProjectManager.h"
#include "Ignis/Rendering/Renderer.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/MaterialFactory.h"
#include "Ignis/Rendering/RenderMesh.h"
#include "EditorShaderCache.h"
#include "EditorPrimitives.h"
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

void EditorLayer::draw_grid(RGTextureHandle color, RGTextureHandle depth)
{
    m_builder.write_render_target(0, color, RGColorAttachmentDesc::load());
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

    const RendererConfig& cfg = Renderer::get_config();
    ComponentInspector::register_defaults();
    EditorPrimitives::init();

    static const PrimShape k_default_shapes[] = {
        PrimShape::Triangle, PrimShape::Quad, PrimShape::Cube, PrimShape::Circle, PrimShape::Sphere, PrimShape::Pyramid,
    };
    constexpr int k_count = static_cast<int>(std::size(k_default_shapes));
    for (int i = 0; i < k_count; ++i)
    {
        Entity e   = EditorPrimitives::spawn(m_active_scene, k_default_shapes[i]);
        auto&  t   = e.get_component<TransformComponent>();
        t.position = Math::Vec3f{(i - (k_count - 1) * 0.5f) * 2.5f, 0.0f, 0.0f};
    }

    GRIViewport*   viewport = Application::get().get_window().get_viewport();
    const uint32_t init_w   = viewport ? viewport->get_width() : 1280u;
    const uint32_t init_h   = viewport ? viewport->get_height() : 720u;
    m_scene_renderer.resize(init_w, init_h);

    m_grid_vs = EditorShaderCache::get().get_or_compile("grid.hlsl", GRIShaderStage::Vertex);
    m_grid_ps = EditorShaderCache::get().get_or_compile("grid.hlsl", GRIShaderStage::Pixel);

    GRIBlendDesc grid_blend;
    grid_blend.enable     = true;
    grid_blend.src_factor = GRIBlendFactor::SrcAlpha;
    grid_blend.dst_factor = GRIBlendFactor::InvSrcAlpha;
    grid_blend.blend_op   = GRIBlendOp::Add;
    grid_blend.src_alpha  = GRIBlendFactor::One;
    grid_blend.dst_alpha  = GRIBlendFactor::InvSrcAlpha;
    grid_blend.alpha_op   = GRIBlendOp::Add;
    GRIRasterDesc grid_raster;
    grid_raster.cull_mode = GRICullMode::None;
    m_grid_material = Renderer::get_material_factory().get_or_create(m_grid_vs, m_grid_ps, "", cfg.render_target_format,
                                                                     cfg.depth_format, {}, grid_raster, grid_blend);

    m_scene_ready = true;

#ifdef ENGINE_IMGUI
    m_panel_registry.register_panel(
        {1, "Scene Tree", []() -> UniquePtr<IPanel> { return create_unique<SceneTreePanel>(); }});
    m_panel_registry.register_panel(
        {2, "Viewport", []() -> UniquePtr<IPanel> { return create_unique<ViewportPanel>(); }});
    m_panel_registry.register_panel(
        {3, "Console", []() -> UniquePtr<IPanel> { return create_unique<ConsolePanel>(); }});
    m_panel_registry.register_panel(
        {4, "Properties", []() -> UniquePtr<IPanel> { return create_unique<PropertyPanel>(); }});

    m_workspace_manager.register_workspace(
        create_unique<SceneEditorWorkspace>(m_active_scene, m_scene_renderer, m_panel_registry));

    m_workspace_manager.activate(1);

    ImGuiLayer::register_drawable(&m_workspace_manager);
#endif
}

void EditorLayer::detach()
{
#ifdef ENGINE_IMGUI
    ImGuiLayer::unregister_drawable(&m_workspace_manager);
#endif
}

void EditorLayer::update(Timestep ts)
{
    EditorAssetManager::get().update(2.0f);

    if (!m_scene_ready)
    {
        return;
    }

    m_active_scene.update(ts);

#ifdef ENGINE_IMGUI
    m_workspace_manager.update(ts);
#endif

    m_scene_renderer.prepare(m_active_scene);
}

void EditorLayer::render()
{
    if (!m_scene_ready)
    {
        return;
    }

#ifdef ENGINE_IMGUI
    auto* ws_data = static_cast<SceneEditorData*>(m_workspace_manager.active_data());
    if (!ws_data)
    {
        return;
    }
    const CameraData& camera = ws_data->camera_data;
#else
    const CameraData camera = CameraData::identity();
#endif

    GRIViewport* viewport = Application::get().get_window().get_viewport();

    Renderer::begin_frame(viewport);
    Renderer::upload_frame_data({camera.view_projection, camera.position});

    GRICommandList& cmd = RenderSystem::get_command_list();

    auto [color_rt, depth_rt] = m_scene_renderer.render_scene(m_active_scene, camera, m_builder);
    draw_grid(color_rt, depth_rt);

    m_builder.execute(cmd);
    Renderer::end_frame();
}

void EditorLayer::event(Event& event)
{
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<KeyPressedEvent>(IG_BIND_EVENT_FN(EditorLayer::key_pressed));
    dispatcher.dispatch<WindowResizeEvent>(IG_BIND_EVENT_FN(EditorLayer::window_resized));
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

bool EditorLayer::window_resized(WindowResizeEvent&)
{
    return false;
}

} // namespace Ignis
