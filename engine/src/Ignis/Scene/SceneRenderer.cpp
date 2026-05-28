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

void SceneRenderer::build_cull_proxies(Scene& scene, const Math::Mat4f& cam_view)
{
    m_cull_proxies.clear();
    m_pool.clear();

    AssetManager&        am        = AssetManager::get();
    const GRIPixelFormat rt_fmt    = Renderer::get_config().render_target_format;
    const GRIPixelFormat depth_fmt = Renderer::get_config().depth_format;

    auto view = scene.registry().view<TransformComponent, MeshComponent>();
    for (auto [entity, transform, mesh_comp] : view.each())
    {
        if (!mesh_comp.is_visible)
        {
            continue;
        }

        SharedPtr<RenderMesh> cached_mesh = Renderer::get_resource_cache().find_mesh(uint64_t(mesh_comp.mesh_id));
        if (!cached_mesh)
        {
            continue;
        }
        const RenderMesh* render_mesh = cached_mesh.get();

        // Resolve material (cache-hit fast path first).
        const Material*   mat       = nullptr;
        GRIPipelineState* depth_pso = nullptr;
        {
            const uint64_t      mat_key    = static_cast<uint64_t>(mesh_comp.material_id);
            SharedPtr<Material> cached_mat = Renderer::get_resource_cache().find_material(mat_key);
            if (!cached_mat)
            {
                SharedPtr<AssetMaterial> asset_mat = am.get_asset_as<AssetMaterial>(mesh_comp.material_id);
                if (!asset_mat)
                {
                    if (am.get_state(mesh_comp.material_id) == AssetState::Unloaded)
                    {
                        am.load_deferred(mesh_comp.material_id);
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

        const Math::Mat4f  world        = transform.to_mat4();
        const Math::Vec3f  world_center = (world * Math::Vec4f(render_mesh->get_bounds_center(), 1.0f)).xyz();
        const Math::Vec3f& s            = transform.scale;
        const float        world_radius = render_mesh->get_bounds_radius() * Math::max(s.x, Math::max(s.y, s.z));

        // view-space Z for depth key quantisation; clamp to [0, k_depth_range]
        const float    raw_z     = cam_view.row(2).x * world_center.x + cam_view.row(2).y * world_center.y +
                                   cam_view.row(2).z * world_center.z + cam_view.row(2).w;
        const float    view_z    = Math::max(0.0f, Math::min(raw_z, k_depth_range));
        const uint32_t depth_u32 = static_cast<uint32_t>(view_z / k_depth_range * float(UINT32_MAX));

        // Pre-pass key: front-to-back (ascending depth), tiebreak by mesh + PSO pointer hash.
        const uint32_t mesh_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(render_mesh) >> 3);
        const uint32_t dpso_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(depth_pso) >> 3);
        const uint64_t depth_key =
            (uint64_t(depth_u32) << 32) | (uint64_t(mesh_id & 0xFFFFu) << 16) | uint64_t(dpso_id & 0xFFFFu);

        // Forward key
        const uint64_t fwd_key = [&]() -> uint64_t
        {
            if (mat->is_transparent())
            {
                // bit63=1 forces transparents after all opaques; ~depth = back-to-front on ascending sort.
                return (uint64_t(1) << 63) | uint64_t(~depth_u32);
            }
            const uint32_t pso_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(mat->get_pipeline_state()) >> 3);
            const uint32_t mat_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(mat) >> 3);
            return (uint64_t(pso_id & 0x7FFFu) << 48) | (uint64_t(mat_id & 0xFFFFu) << 32) | uint64_t(depth_u32);
        }();

        // Hot cull proxy (entity_index = parallel index into m_pool).
        CullProxy proxy;
        proxy.world_center = world_center;
        proxy.world_radius = world_radius;
        proxy.entity_index = static_cast<uint32_t>(m_pool.size());
        m_cull_proxies.push_back(proxy);

        VisibleItem item;
        item.mesh      = render_mesh;
        item.material  = mat;
        item.depth_pso = depth_pso;
        item.world     = world;
        item.depth_key = depth_key;
        item.fwd_key   = fwd_key;
        m_pool.push_back(item);
    }
}

void SceneRenderer::build_commands()
{
    m_depth_cmds.clear();
    m_fwd_cmds.clear();
    m_instance_data.clear();

    if (m_visible.empty())
    {
        return;
    }

    // --- Depth pre-pass commands (opaques only, sorted front-to-back) ---
    std::sort(m_visible.begin(), m_visible.end(),
              [](const VisibleItem& a, const VisibleItem& b) { return a.depth_key < b.depth_key; });

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_visible.size()); ++i)
    {
        const VisibleItem& item = m_visible[i];
        if (item.fwd_key >> 63)
        {
            continue; // transparent: skip depth pre-pass
        }
        if (m_instance_data.size() >= k_max_instances)
        {
            IG_CORE_ASSERT(false, "SceneRenderer: k_max_instances exceeded in depth pre-pass");
            break;
        }

        const bool same_batch =
            !m_depth_cmds.empty() && item.mesh == m_depth_cmds.back().mesh && item.depth_pso == m_depth_cmds.back().pso;
        if (!same_batch)
        {
            RenderCommand rc;
            rc.sort_key       = item.depth_key;
            rc.mesh           = item.mesh;
            rc.material       = nullptr;
            rc.pso            = item.depth_pso;
            rc.instance_count = 0;
            rc.base_instance  = static_cast<uint32_t>(m_instance_data.size());
            m_depth_cmds.push_back(rc);
        }
        m_depth_cmds.back().instance_count++;

        GPUInstanceData gid;
        gid.world_matrix   = item.world;
        gid.material_index = 0;
        gid.padding[0] = gid.padding[1] = gid.padding[2] = 0;
        m_instance_data.push_back(gid);
    }

    // --- Forward commands (opaques state-minimised, then transparents back-to-front) ---
    const uint32_t fwd_instance_base = static_cast<uint32_t>(m_instance_data.size());

    std::sort(m_visible.begin(), m_visible.end(),
              [](const VisibleItem& a, const VisibleItem& b) { return a.fwd_key < b.fwd_key; });

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_visible.size()); ++i)
    {
        const VisibleItem& item        = m_visible[i];
        const bool         transparent = (item.fwd_key >> 63) != 0;

        if (m_instance_data.size() >= k_max_instances)
        {
            IG_CORE_ASSERT(false, "SceneRenderer: k_max_instances exceeded in forward pass");
            break;
        }

        // Transparents are never coalesced: draw order must be preserved per-item.
        const bool same_batch = !transparent && !m_fwd_cmds.empty() && item.mesh == m_fwd_cmds.back().mesh &&
                                item.material->get_pipeline_state() == m_fwd_cmds.back().pso &&
                                item.material == m_fwd_cmds.back().material;
        if (!same_batch)
        {
            RenderCommand rc;
            rc.sort_key       = item.fwd_key;
            rc.mesh           = item.mesh;
            rc.material       = item.material;
            rc.pso            = item.material->get_pipeline_state();
            rc.instance_count = 0;
            rc.base_instance  = static_cast<uint32_t>(m_instance_data.size());
            m_fwd_cmds.push_back(rc);
        }
        m_fwd_cmds.back().instance_count++;

        GPUInstanceData gid;
        gid.world_matrix   = item.world;
        gid.material_index = 0;
        gid.padding[0] = gid.padding[1] = gid.padding[2] = 0;
        m_instance_data.push_back(gid);
    }
    (void)fwd_instance_base;

    // Upload instance data to GPU.
    if (!m_instance_data.empty())
    {
        if (!m_instance_buffer)
        {
            GRIBufferDesc desc;
            desc.size         = k_max_instances * static_cast<uint32_t>(sizeof(GPUInstanceData));
            desc.usage        = static_cast<GRIBufferUsage>(static_cast<uint32_t>(GRIBufferUsage::VertexBuffer) |
                                                            static_cast<uint32_t>(GRIBufferUsage::Dynamic));
            m_instance_buffer = RenderSystem::get_gri()->create_buffer(desc);
        }
        RenderSystem::get_gri()->update_buffer(m_instance_buffer.get(), m_instance_data.data(),
                                               static_cast<uint32_t>(m_instance_data.size() * sizeof(GPUInstanceData)));
    }
}

void SceneRenderer::render_scene(Scene& scene, const CameraData& camera, RGBuilder& builder, RGTextureHandle backbuffer,
                                 AssetID scene_texture_id)
{
    const Math::Mat4f& cam_view = camera.view;

    build_cull_proxies(scene, cam_view);

    // Frustum cull: iterate hot proxy array, copy passing items from m_pool.
    m_visible.clear();
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_cull_proxies.size()); ++i)
    {
        const CullProxy& proxy = m_cull_proxies[i];
        if (Math::frustum_contains_sphere(camera.frustum, proxy.world_center, proxy.world_radius))
        {
            m_visible.push_back(m_pool[proxy.entity_index]);
        }
    }

    // NOTE: Do we want to do batching in terms of instancing? Maybe only when we have the same mesh, but later when we
    // have different meshes! Especially when we add a PBR shader!
    build_commands();

    RGTextureHandle depth = builder.import_viewport_depth();

    // --- Depth pre-pass ---
    builder.write_depth_stencil(depth, {GRILoadAction::Clear, GRIStoreAction::Store, 1.0f});
    builder.add_pass("DepthPrePass",
                     [this](GRICommandList& cmd)
                     {
                         if (m_depth_cmds.empty() || !m_instance_buffer)
                         {
                             return;
                         }
                         Renderer::bind_frame_data(cmd);
                         cmd.set_vertex_buffer(m_instance_buffer.get(), 0, k_instance_buffer_slot);

                         GRIPipelineState* current_pso = nullptr;
                         for (const RenderCommand& rc : m_depth_cmds)
                         {
                             if (rc.pso != current_pso)
                             {
                                 cmd.set_graphics_pipeline_state(rc.pso);
                                 current_pso = rc.pso;
                             }
                             cmd.set_vertex_buffer(rc.mesh->get_vertex_buffer());
                             cmd.set_index_buffer(rc.mesh->get_index_buffer(), rc.mesh->get_index_format());
                             cmd.draw_indexed_primitives_instanced(rc.mesh->get_index_count(), rc.instance_count,
                                                                   rc.base_instance);
                         }
                     });

    // Explicit read dependency: DepthPrePass must complete before ForwardScene.
    builder.read_texture(depth);

    // --- Forward scene pass ---
    ScenePassParams* params = builder.alloc_params<ScenePassParams>();
    params->scene_texture   = resolve_texture(scene_texture_id);

    builder.write_render_target(0, backbuffer, RGColorAttachmentDesc::clear({0.1f, 0.1f, 0.1f, 1.0f}));
    builder.read_depth_stencil(depth, {GRILoadAction::Load, GRIStoreAction::DontCare, 1.0f});
    builder.add_pass(
        "ForwardScene", params,
        [this](ScenePassParams* p, GRICommandList& cmd)
        {
            if (m_fwd_cmds.empty() || !m_instance_buffer)
            {
                return;
            }
            Renderer::bind_frame_data(cmd);
            cmd.set_texture(p->scene_texture, 0, GRIShaderStage::Pixel);
            cmd.set_vertex_buffer(m_instance_buffer.get(), 0, k_instance_buffer_slot);

            GRIPipelineState* current_pso = nullptr;
            const Material*   current_mat = nullptr;
            for (const RenderCommand& rc : m_fwd_cmds)
            {
                if (rc.pso != current_pso)
                {
                    cmd.set_graphics_pipeline_state(rc.pso);
                    current_pso = rc.pso;
                }
                if (rc.material != current_mat)
                {
                    if (rc.material->get_params_buffer())
                    {
                        cmd.set_uniform_buffer(rc.material->get_params_buffer(),
                                               static_cast<uint32_t>(UniformSlot::MaterialArgs), GRIShaderStage::Pixel);
                    }
                    current_mat = rc.material;
                }
                cmd.set_vertex_buffer(rc.mesh->get_vertex_buffer());
                cmd.set_index_buffer(rc.mesh->get_index_buffer(), rc.mesh->get_index_format());
                cmd.draw_indexed_primitives_instanced(rc.mesh->get_index_count(), rc.instance_count, rc.base_instance);
            }
        });
}

} // namespace Ignis
