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
#include "Ignis/Events/MouseEvent.h"
#include <cmath>

namespace Ignis
{

namespace
{

struct PrimVertex
{
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
};

constexpr uint64_t k_prim_material_key = 0xED17'0000'0000'0000ULL;
constexpr uint64_t k_prim_mesh_keys[6] = {
    0xED17'0000'0000'0001ULL, 0xED17'0000'0000'0002ULL, 0xED17'0000'0000'0003ULL,
    0xED17'0000'0000'0004ULL, 0xED17'0000'0000'0005ULL, 0xED17'0000'0000'0006ULL,
};

static const PrimVertex k_tri_verts[] = {
    {0.000f, 0.500f, 0.f, 0, 0, 1, 0.5f, 0.0f},
    {-0.433f, -0.250f, 0.f, 0, 0, 1, 0.0f, 1.0f},
    {0.433f, -0.250f, 0.f, 0, 0, 1, 1.0f, 1.0f},
};
static const uint32_t k_tri_indices[] = {0, 1, 2};

static const PrimVertex k_quad_verts[] = {
    {-0.5f, -0.5f, 0.f, 0, 0, 1, 0, 1},
    {0.5f, -0.5f, 0.f, 0, 0, 1, 1, 1},
    {0.5f, 0.5f, 0.f, 0, 0, 1, 1, 0},
    {-0.5f, 0.5f, 0.f, 0, 0, 1, 0, 0},
};
static const uint32_t k_quad_indices[] = {0, 1, 2, 0, 2, 3};

static const PrimVertex k_cube_verts[] = {
    {0.5f, -0.5f, -0.5f, 1, 0, 0, 0, 1},   {0.5f, 0.5f, -0.5f, 1, 0, 0, 0, 0},    {0.5f, 0.5f, 0.5f, 1, 0, 0, 1, 0},
    {0.5f, -0.5f, 0.5f, 1, 0, 0, 1, 1},    {-0.5f, -0.5f, 0.5f, -1, 0, 0, 0, 1},  {-0.5f, 0.5f, 0.5f, -1, 0, 0, 0, 0},
    {-0.5f, 0.5f, -0.5f, -1, 0, 0, 1, 0},  {-0.5f, -0.5f, -0.5f, -1, 0, 0, 1, 1}, {0.5f, 0.5f, 0.5f, 0, 1, 0, 0, 1},
    {0.5f, 0.5f, -0.5f, 0, 1, 0, 0, 0},    {-0.5f, 0.5f, -0.5f, 0, 1, 0, 1, 0},   {-0.5f, 0.5f, 0.5f, 0, 1, 0, 1, 1},
    {0.5f, -0.5f, -0.5f, 0, -1, 0, 0, 1},  {0.5f, -0.5f, 0.5f, 0, -1, 0, 0, 0},   {-0.5f, -0.5f, 0.5f, 0, -1, 0, 1, 0},
    {-0.5f, -0.5f, -0.5f, 0, -1, 0, 1, 1}, {-0.5f, -0.5f, 0.5f, 0, 0, 1, 0, 1},   {0.5f, -0.5f, 0.5f, 0, 0, 1, 1, 1},
    {0.5f, 0.5f, 0.5f, 0, 0, 1, 1, 0},     {-0.5f, 0.5f, 0.5f, 0, 0, 1, 0, 0},    {0.5f, -0.5f, -0.5f, 0, 0, -1, 0, 1},
    {-0.5f, -0.5f, -0.5f, 0, 0, -1, 1, 1}, {-0.5f, 0.5f, -0.5f, 0, 0, -1, 1, 0},  {0.5f, 0.5f, -0.5f, 0, 0, -1, 0, 0},
};
static const uint32_t k_cube_indices[] = {
    0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
    12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
};

static const PrimVertex k_pyramid_verts[] = {
    {0.5f, -0.5f, 0.5f, 0, -1, 0, 1, 0},
    {-0.5f, -0.5f, 0.5f, 0, -1, 0, 0, 0},
    {-0.5f, -0.5f, -0.5f, 0, -1, 0, 0, 1},
    {0.5f, -0.5f, -0.5f, 0, -1, 0, 1, 1},
    {-0.5f, -0.5f, 0.5f, 0, 0.4472f, 0.8944f, 0, 0},
    {0.5f, -0.5f, 0.5f, 0, 0.4472f, 0.8944f, 1, 0},
    {0.0f, 0.5f, 0.0f, 0, 0.4472f, 0.8944f, 0.5f, 1},
    {0.5f, -0.5f, 0.5f, 0.8944f, 0.4472f, 0, 0, 0},
    {0.5f, -0.5f, -0.5f, 0.8944f, 0.4472f, 0, 1, 0},
    {0.0f, 0.5f, 0.0f, 0.8944f, 0.4472f, 0, 0.5f, 1},
    {0.5f, -0.5f, -0.5f, 0, 0.4472f, -0.8944f, 0, 0},
    {-0.5f, -0.5f, -0.5f, 0, 0.4472f, -0.8944f, 1, 0},
    {0.0f, 0.5f, 0.0f, 0, 0.4472f, -0.8944f, 0.5f, 1},
    {-0.5f, -0.5f, -0.5f, -0.8944f, 0.4472f, 0, 0, 0},
    {-0.5f, -0.5f, 0.5f, -0.8944f, 0.4472f, 0, 1, 0},
    {0.0f, 0.5f, 0.0f, -0.8944f, 0.4472f, 0, 0.5f, 1},
};
static const uint32_t k_pyramid_indices[] = {
    0, 1, 2, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
};

static SharedPtr<RenderMesh> build_circle(int segs = 32)
{
    Vector<PrimVertex> verts;
    Vector<uint32_t>   indices;
    verts.reserve(segs + 1);
    indices.reserve(segs * 3);

    verts.push_back({0, 0, 0, 0, 0, 1, 0.5f, 0.5f});
    for (int i = 0; i < segs; ++i)
    {
        const float a  = i * 2.0f * 3.14159265f / segs;
        const float cx = std::cos(a) * 0.5f;
        const float cy = std::sin(a) * 0.5f;
        verts.push_back({cx, cy, 0, 0, 0, 1, 0.5f + cx, 0.5f + cy});
    }
    for (int i = 0; i < segs; ++i)
    {
        indices.push_back(0);
        indices.push_back(static_cast<uint32_t>(1 + i));
        indices.push_back(static_cast<uint32_t>(1 + (i + 1) % segs));
    }
    return RenderMesh::create(verts.data(), static_cast<uint32_t>(verts.size() * sizeof(PrimVertex)), indices.data(),
                              static_cast<uint32_t>(indices.size()));
}

static SharedPtr<RenderMesh> build_sphere(int rings = 16, int segs = 32)
{
    Vector<PrimVertex> verts;
    Vector<uint32_t>   indices;
    verts.reserve((rings + 1) * (segs + 1));
    indices.reserve(rings * segs * 6);

    for (int r = 0; r <= rings; ++r)
    {
        const float phi = 3.14159265f * r / rings;
        const float sp  = std::sin(phi);
        const float cp  = std::cos(phi);
        for (int s = 0; s <= segs; ++s)
        {
            const float theta = 2.0f * 3.14159265f * s / segs;
            const float nx    = sp * std::cos(theta);
            const float ny    = cp;
            const float nz    = sp * std::sin(theta);
            verts.push_back({0.5f * nx, 0.5f * ny, 0.5f * nz, nx, ny, nz, static_cast<float>(s) / segs,
                             static_cast<float>(r) / rings});
        }
    }
    for (int r = 0; r < rings; ++r)
    {
        for (int s = 0; s < segs; ++s)
        {
            const uint32_t i0 = static_cast<uint32_t>(r * (segs + 1) + s);
            const uint32_t i1 = i0 + 1;
            const uint32_t i2 = i0 + static_cast<uint32_t>(segs + 1);
            const uint32_t i3 = i2 + 1;
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }
    return RenderMesh::create(verts.data(), static_cast<uint32_t>(verts.size() * sizeof(PrimVertex)), indices.data(),
                              static_cast<uint32_t>(indices.size()));
}

} // namespace

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
    m_scene_texture_id         = assets.import_texture("test.png");
    assets.load_deferred(m_scene_texture_id);

    const RendererConfig&   cfg     = Renderer::get_config();
    SharedPtr<RenderShader> prim_vs = EditorShaderCache::get().get_or_compile("primitive.hlsl", GRIShaderStage::Vertex);
    SharedPtr<RenderShader> prim_ps = EditorShaderCache::get().get_or_compile("primitive.hlsl", GRIShaderStage::Pixel);
    GRIRasterDesc           prim_raster;
    prim_raster.cull_mode        = GRICullMode::None;
    SharedPtr<Material> prim_mat = Renderer::get_material_factory().get_or_create(
        prim_vs, prim_ps, "standard_mesh", cfg.render_target_format, cfg.depth_format, {}, prim_raster, {});
    Renderer::get_resource_cache().register_material(k_prim_material_key, prim_mat);

    Renderer::get_resource_cache().register_mesh(
        k_prim_mesh_keys[0], RenderMesh::create(k_tri_verts, sizeof(k_tri_verts), k_tri_indices, 3));
    Renderer::get_resource_cache().register_mesh(
        k_prim_mesh_keys[1], RenderMesh::create(k_quad_verts, sizeof(k_quad_verts), k_quad_indices, 6));
    Renderer::get_resource_cache().register_mesh(
        k_prim_mesh_keys[2], RenderMesh::create(k_cube_verts, sizeof(k_cube_verts), k_cube_indices, 36));
    Renderer::get_resource_cache().register_mesh(k_prim_mesh_keys[3], build_circle());
    Renderer::get_resource_cache().register_mesh(k_prim_mesh_keys[4], build_sphere());
    Renderer::get_resource_cache().register_mesh(
        k_prim_mesh_keys[5], RenderMesh::create(k_pyramid_verts, sizeof(k_pyramid_verts), k_pyramid_indices, 18));

    constexpr int k_count = 6;
    for (int i = 0; i < k_count; ++i)
    {
        TransformComponent trans;
        trans.position = Math::Vec3f{(i - (k_count - 1) * 0.5f) * 2.5f, 0.0f, 0.0f};

        Entity e = m_active_scene.create_entity();
        e.add_component<TransformComponent>(trans);

        MeshRendererComponent mrc;
        mrc.mesh_id = AssetID{k_prim_mesh_keys[i]};
        e.add_component<MeshRendererComponent>(mrc);

        MaterialComponent matc;
        matc.material_id = AssetID{k_prim_material_key};
        e.add_component<MaterialComponent>(matc);
    }

    m_fly_camera.focus_on(Math::Vec3f{0.0f, 0.0f, 0.0f}, Math::Vec3f{0.0f, 4.0f, -12.0f});

    GRIViewport* viewport = Application::get().get_window().get_viewport();
    const float  aspect   = (viewport && viewport->get_height() > 0)
                                ? static_cast<float>(viewport->get_width()) / static_cast<float>(viewport->get_height())
                                : 1.778f;
    m_fly_camera.set_aspect(aspect);

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
    draw_grid(bb);

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
