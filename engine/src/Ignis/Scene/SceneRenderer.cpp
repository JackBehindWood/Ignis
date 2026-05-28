#include "igpch.h"
#include "SceneRenderer.h"
#include "Components/Components.h"
#include "Ignis/Asset/AssetManager.h"
#include "Ignis/Asset/AssetMesh.h"
#include "Ignis/Asset/AssetMaterial.h"
#include "Ignis/Asset/AssetTexture2D.h"
#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/Renderer.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/RenderTexture2D.h"
#include "Ignis/Rendering/RenderGraph/RGBuilder.h"
#include "Ignis/Rendering/Shaders/ShaderCache.h"
#include "Ignis/Rendering/Shaders/ShaderCompiler.h"
#include "Ignis/Rendering/VertexDeclarationRegistry.h"

namespace Ignis
{

namespace Utils
{
static void emit_depth(GRICommandList& cmd, const FrameDrawItem& item)
{
    cmd.set_graphics_pipeline_state(item.depth_pso);
    cmd.set_vertex_buffer(item.mesh->get_vertex_buffer());
    cmd.set_index_buffer(item.mesh->get_index_buffer(), item.mesh->get_index_format());
    Renderer::bind_transform(cmd, item.world.data(), sizeof(Math::Mat4f));
    cmd.draw_indexed_primitives(item.mesh->get_index_count());
}

static void emit(GRICommandList& cmd, const FrameDrawItem& item)
{
    cmd.set_graphics_pipeline_state(item.material->get_pipeline_state());
    if (item.material->get_params_buffer())
    {
        cmd.set_uniform_buffer(item.material->get_params_buffer(), static_cast<uint32_t>(UniformSlot::MaterialArgs),
                               GRIShaderStage::Pixel);
    }
    cmd.set_vertex_buffer(item.mesh->get_vertex_buffer());
    cmd.set_index_buffer(item.mesh->get_index_buffer(), item.mesh->get_index_format());
    Renderer::bind_transform(cmd, item.world.data(), sizeof(Math::Mat4f));
    cmd.draw_indexed_primitives(item.mesh->get_index_count());
}
} // namespace Utils

GRITexture2D* SceneRenderer::resolve_texture(AssetID id)
{
    AssetManager&             am  = AssetManager::get();
    SharedPtr<AssetTexture2D> tex = am.get_asset_as<AssetTexture2D>(id);
    if (!tex)
    {
        tex = static_pointer_cast<AssetTexture2D>(am.get_fallback(AssetType::Texture2D));
    }
    if (!tex)
    {
        return nullptr;
    }

    const uint64_t             key    = static_cast<uint64_t>(tex->get_id());
    SharedPtr<RenderTexture2D> cached = Renderer::get_resource_cache().find_texture(key);
    if (!cached)
    {
        GRITexture2DDesc desc;
        desc.width             = tex->get_width();
        desc.height            = tex->get_height();
        desc.num_mip_levels    = 1;
        desc.format            = static_cast<GRIPixelFormat>(tex->get_format());
        desc.initial_data      = tex->get_pixels().data();
        desc.initial_data_size = static_cast<uint32_t>(tex->get_pixels().size());

        if (GRITexture2DPtr gri_tex = RenderSystem::get_gri()->create_texture2d(desc))
        {
            cached = create_shared<RenderTexture2D>(std::move(gri_tex), desc.width, desc.height, desc.format);
            Renderer::get_resource_cache().register_texture(key, cached);
        }
    }
    return cached ? cached->get_texture() : nullptr;
}

void SceneRenderer::prepare(Scene& scene)
{
    AssetManager& am   = AssetManager::get();
    auto          view = scene.registry().view<MeshComponent>();

    for (auto [entity, mesh_comp] : view.each())
    {
        const uint64_t key = static_cast<uint64_t>(mesh_comp.mesh_id);
        if (Renderer::get_resource_cache().find_mesh(key))
        {
            continue;
        }

        SharedPtr<AssetMesh> asset_mesh = am.get_asset_as<AssetMesh>(mesh_comp.mesh_id);
        if (!asset_mesh)
        {
            if (am.get_state(mesh_comp.mesh_id) == AssetState::Unloaded)
            {
                am.load_deferred(mesh_comp.mesh_id);
            }
            continue;
        }

        Math::Vec3f            bounds_center{};
        float                  bounds_radius = 1.0e30f;
        const uint32_t         stride        = asset_mesh->get_vertex_stride();
        const Vector<uint8_t>& verts         = asset_mesh->get_vertices();
        if (stride >= sizeof(Math::Vec3f) && !verts.empty())
        {
            const uint32_t count = static_cast<uint32_t>(verts.size()) / stride;
            Math::Vec3f    mn{1.0e30f, 1.0e30f, 1.0e30f};
            Math::Vec3f    mx{-1.0e30f, -1.0e30f, -1.0e30f};
            for (uint32_t i = 0; i < count; ++i)
            {
                const Math::Vec3f& p = *reinterpret_cast<const Math::Vec3f*>(verts.data() + i * stride);
                mn.x                 = Math::min(mn.x, p.x);
                mn.y                 = Math::min(mn.y, p.y);
                mn.z                 = Math::min(mn.z, p.z);
                mx.x                 = Math::max(mx.x, p.x);
                mx.y                 = Math::max(mx.y, p.y);
                mx.z                 = Math::max(mx.z, p.z);
            }
            Math::Vec3f bounds_size = mx - mn;
            bounds_center           = mn + bounds_size * 0.5f;
            bounds_radius           = Math::length(bounds_size) * 0.5f;
        }

        SharedPtr<RenderMesh> render_mesh =
            RenderMesh::create(verts.data(), static_cast<uint32_t>(verts.size()), asset_mesh->get_indices().data(),
                               static_cast<uint32_t>(asset_mesh->get_indices().size()), GRIIndexFormat::Uint32,
                               bounds_center, bounds_radius);
        Renderer::get_resource_cache().register_mesh(key, render_mesh);
    }
}

void SceneRenderer::render_scene(Scene& scene, const CameraData& camera, RGBuilder& builder, RGTextureHandle backbuffer,
                                 AssetID scene_texture_id)
{
    m_opaque.clear();
    m_transparent.clear();

    const Math::Frustum& frustum  = camera.frustum;
    const Math::Mat4f&   cam_view = camera.view;

    const GRIPixelFormat rt_fmt    = Renderer::get_config().render_target_format;
    const GRIPixelFormat depth_fmt = Renderer::get_config().depth_format;

    auto view = scene.registry().view<TransformComponent, MeshComponent>();
    for (auto [entity, transform, mesh_comp] : view.each())
    {
        if (!mesh_comp.is_visible)
        {
            continue;
        }

        const Math::Mat4f world = transform.to_mat4();

        // --- Mesh resolve (cache-hit only; prepare() handles uploads) ---
        const RenderMesh* render_mesh = nullptr;
        {
            SharedPtr<RenderMesh> cached = Renderer::get_resource_cache().find_mesh(uint64_t(mesh_comp.mesh_id));
            if (!cached)
            {
                continue;
            }
            render_mesh = cached.get();
        }

        // --- Frustum cull ---
        {
            const Math::Vec3f  world_center = (world * Math::Vec4f(render_mesh->get_bounds_center(), 1.0f)).xyz();
            const Math::Vec3f& s            = transform.scale;
            const float        world_radius = render_mesh->get_bounds_radius() * Math::max(s.x, Math::max(s.y, s.z));
            if (!Math::frustum_contains_sphere(frustum, world_center, world_radius))
            {
                continue;
            }
        }

        // --- Material resolve (via MaterialFactory) ---
        const Material*   mat       = nullptr;
        GRIPipelineState* depth_pso = nullptr;
        {
            const uint64_t      mat_key    = static_cast<uint64_t>(mesh_comp.material_id);
            SharedPtr<Material> cached_mat = Renderer::get_resource_cache().find_material(mat_key);
            if (!cached_mat)
            {
                AssetManager&            amv2      = AssetManager::get();
                SharedPtr<AssetMaterial> asset_mat = amv2.get_asset_as<AssetMaterial>(mesh_comp.material_id);
                if (!asset_mat)
                {
                    if (amv2.get_state(mesh_comp.material_id) == AssetState::Unloaded)
                    {
                        amv2.load_deferred(mesh_comp.material_id);
                    }
                    continue;
                }

                ShaderCompilerOptions opts;
                opts.stages[0] = {GRIShaderStage::Vertex};
                opts.stages[1] = {GRIShaderStage::Pixel};
                opts.count     = 2;

                SharedPtr<RenderShader> vs =
                    ShaderCache::get().get_or_compile(asset_mat->get_shader_source(), GRIShaderStage::Vertex, opts);
                SharedPtr<RenderShader> ps =
                    ShaderCache::get().get_or_compile(asset_mat->get_shader_source(), GRIShaderStage::Pixel, opts);
                if (!vs || !ps)
                {
                    continue;
                }

                const String& layout = asset_mat->get_vertex_layout();
                if (!VertexDeclarationRegistry::get().find(layout))
                {
                    continue;
                }

                GRIBufferPtr           params_buffer;
                const Vector<uint8_t>& param_data = asset_mat->get_param_data();
                if (!param_data.empty())
                {
                    GRIBufferDesc buf_desc;
                    buf_desc.size  = static_cast<uint32_t>(param_data.size());
                    buf_desc.usage = GRIBufferUsage::UniformBuffer;
                    params_buffer  = RenderSystem::get_gri()->create_buffer(buf_desc, param_data.data());
                }

                cached_mat = Renderer::get_material_factory().create_with_params(
                    vs, ps, layout, rt_fmt, depth_fmt, false, GRIBlendMode::None, std::move(params_buffer));
                Renderer::get_resource_cache().register_material(mat_key, cached_mat);
            }
            mat       = cached_mat.get();
            depth_pso = cached_mat->get_depth_pso();
        }

        const Math::Vec3f pos = transform.position;
        const float       depth =
            cam_view.row(2).x * pos.x + cam_view.row(2).y * pos.y + cam_view.row(2).z * pos.z + cam_view.row(2).w;

        FrameDrawItem item;
        item.world     = world;
        item.mesh      = render_mesh;
        item.material  = mat;
        item.depth_pso = depth_pso;
        item.depth     = depth;

        if (mat->is_transparent())
        {
            m_transparent.push_back(item);
        }
        else
        {
            m_opaque.push_back(item);
        }
    }

    // --- Sort ---
    std::sort(m_opaque.begin(), m_opaque.end(),
              [](const FrameDrawItem& a, const FrameDrawItem& b) { return a.material < b.material; });
    std::sort(m_transparent.begin(), m_transparent.end(),
              [](const FrameDrawItem& a, const FrameDrawItem& b) { return a.depth > b.depth; });

    RGTextureHandle depth = builder.import_viewport_depth();

    // --- Depth pre-pass ---
    builder.write_depth_stencil(depth, {GRILoadAction::Clear, GRIStoreAction::Store, 1.0f});
    builder.add_pass("DepthPrePass",
                     [this](GRICommandList& cmd)
                     {
                         Renderer::bind_frame_data(cmd);
                         for (const FrameDrawItem& item : m_opaque)
                         {
                             Utils::emit_depth(cmd, item);
                         }
                     });

    // Explicit read dependency: DepthPrePass must complete before ForwardScene.
    builder.read_texture(depth);

    // --- Forward scene pass ---
    ScenePassParams* params = builder.alloc_params<ScenePassParams>();
    params->scene_texture   = resolve_texture(scene_texture_id);

    builder.write_render_target(0, backbuffer, RGColorAttachmentDesc::clear({0.1f, 0.1f, 0.1f, 1.0f}));
    builder.read_depth_stencil(depth, {GRILoadAction::Load, GRIStoreAction::DontCare, 1.0f});
    builder.add_pass("ForwardScene", params,
                     [this](ScenePassParams* p, GRICommandList& cmd)
                     {
                         Renderer::bind_frame_data(cmd);
                         cmd.set_texture(p->scene_texture, 0, GRIShaderStage::Pixel);

                         for (const FrameDrawItem& item : m_opaque)
                         {
                             Utils::emit(cmd, item);
                         }
                         for (const FrameDrawItem& item : m_transparent)
                         {
                             Utils::emit(cmd, item);
                         }
                     });
}

} // namespace Ignis
