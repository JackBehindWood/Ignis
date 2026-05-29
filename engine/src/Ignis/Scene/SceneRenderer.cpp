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
#include "Ignis/Rendering/VertexDeclarationRegistry.h"
#include "Ignis/Rendering/StaticGeometryBatcher.h"

namespace Ignis
{

namespace
{

constexpr uint64_t k_fallback_material_key = 0xFFFF'FFFF'FFFF'FFFEull;

constexpr const char* k_fallback_hlsl = R"(
#pragma pack_matrix(column_major)
struct FrameUniforms { float4x4 view_projection; };
struct GPUInstanceData { float4x4 world_matrix; uint material_index; uint3 padding; };
struct VertexIn { float3 position : POSITION; float3 normal : NORMAL; float2 uv : TEXCOORD; };
struct VertexOut { float4 position : SV_POSITION; };
ConstantBuffer<FrameUniforms> g_frame : register(b0);
StructuredBuffer<GPUInstanceData> g_instances : register(t0, space28);
VertexOut VSMain(VertexIn input, uint instanceID : SV_InstanceID)
{
    VertexOut o;
    o.position = mul(g_frame.view_projection, mul(g_instances[instanceID].world_matrix, float4(input.position, 1.0)));
    return o;
}
float4 PSMain(VertexOut input) : SV_TARGET { return float4(1.0, 0.0, 1.0, 1.0); }
)";

} // namespace

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

// #NOTE: Fallback shader seems to fail with compiling!
void SceneRenderer::prepare(Scene& scene)
{
    if (!Renderer::get_resource_cache().find_material(k_fallback_material_key))
    {
        m_fallback_material = nullptr;
    }
    if (!m_fallback_material)
    {
        ShaderCompilerOptions fallback_opts;
        fallback_opts.stages[0] = {GRIShaderStage::Vertex};
        fallback_opts.stages[1] = {GRIShaderStage::Pixel};
        fallback_opts.count     = 2;

        const RendererConfig&   cfg = Renderer::get_config();
        SharedPtr<RenderShader> vs  = ShaderCache::get().get_or_compile(String(k_fallback_hlsl), "_fallback",
                                                                        GRIShaderStage::Vertex, fallback_opts);
        SharedPtr<RenderShader> ps  = ShaderCache::get().get_or_compile(String(k_fallback_hlsl), "_fallback",
                                                                        GRIShaderStage::Pixel, fallback_opts);
        if (vs && ps)
        {
            m_fallback_material = Renderer::get_material_factory().get_or_create(
                vs, ps, "standard_mesh", cfg.render_target_format, cfg.depth_format, {}, {}, {});
            m_fallback_material_id =
                Renderer::get_resource_cache().register_material(k_fallback_material_key, m_fallback_material);
        }
    }

    AssetManager&        am        = AssetManager::get();
    const GRIPixelFormat rt_fmt    = Renderer::get_config().render_target_format;
    const GRIPixelFormat depth_fmt = Renderer::get_config().depth_format;

    // Mesh resolution — register with batcher; bounds read from cook-time fields.
    {
        auto view = scene.registry().view<MeshRendererComponent>();
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

            const MeshSlot slot = StaticGeometryBatcher::get().register_mesh(
                key, asset_mesh->get_vertices().data(), static_cast<uint32_t>(asset_mesh->get_vertices().size()),
                asset_mesh->get_indices().data(), static_cast<uint32_t>(asset_mesh->get_indices().size()));

            SharedPtr<RenderMesh> render_mesh =
                RenderMesh::create_batched(asset_mesh->get_bounds_center(), asset_mesh->get_bounds_radius(), slot);
            Renderer::get_resource_cache().register_mesh(key, std::move(render_mesh));
        }
        StaticGeometryBatcher::get().flush_to_gpu();
    }

    // Material & PSO resolution — async-only path.
    // build_cull_proxies is a strict cache-hit-only zone; all compilation happens here.
    {
        auto view = scene.registry().view<MaterialComponent>();
        for (auto [entity, mat_comp] : view.each())
        {
            const uint64_t key = static_cast<uint64_t>(mat_comp.material_id);
            if (Renderer::get_resource_cache().find_material(key))
            {
                continue;
            }

            SharedPtr<AssetMaterial> asset_mat = am.get_asset_as<AssetMaterial>(mat_comp.material_id);
            if (!asset_mat)
            {
                if (am.get_state(mat_comp.material_id) == AssetState::Unloaded)
                {
                    am.load_deferred(mat_comp.material_id);
                }
                continue;
            }

            ShaderCompilerOptions opts;
            opts.stages[0] = {GRIShaderStage::Vertex};
            opts.stages[1] = {GRIShaderStage::Pixel};
            opts.count     = 2;

            // get_or_compile_async: returns nullptr if PSO not yet baked (Phase 2+).
            // Entity will receive fallback material in build_cull_proxies for this frame.
            SharedPtr<RenderShader> vs =
                ShaderCache::get().get_or_compile_async(asset_mat->get_shader_source(), GRIShaderStage::Vertex, opts);
            SharedPtr<RenderShader> ps =
                ShaderCache::get().get_or_compile_async(asset_mat->get_shader_source(), GRIShaderStage::Pixel, opts);
            if (!vs || !ps)
            {
                continue;
            }

            const String& layout = asset_mat->get_vertex_layout();
            if (!VertexDeclarationRegistry::get().find(layout))
            {
                continue;
            }

            GRIDepthStencilDesc ds;
            GRIRasterDesc       raster;
            raster.cull_mode = asset_mat->is_two_sided() ? GRICullMode::None : GRICullMode::Back;

            GRIBlendDesc blend;
            if (asset_mat->is_transparent())
            {
                blend.enable     = true;
                blend.src_factor = GRIBlendFactor::SrcAlpha;
                blend.dst_factor = GRIBlendFactor::InvSrcAlpha;
                blend.blend_op   = GRIBlendOp::Add;
                blend.src_alpha  = GRIBlendFactor::One;
                blend.dst_alpha  = GRIBlendFactor::InvSrcAlpha;
                blend.alpha_op   = GRIBlendOp::Add;
                ds.depth_write   = false;
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

            SharedPtr<Material> material = Renderer::get_material_factory().create_with_params(
                vs, ps, layout, rt_fmt, depth_fmt, ds, raster, blend, std::move(params_buffer));
            Renderer::get_resource_cache().register_material(key, std::move(material));
        }
    }
}

// Strict cache-hit-only zone. No shader compilation, PSO creation, or MTL::Function lookup may occur here.
void SceneRenderer::build_cull_proxies(Scene& scene, const Math::Mat4f& cam_view)
{
    m_cull_proxies.clear();
    m_pool.clear();

    auto view = scene.registry().view<TransformComponent, MeshRendererComponent>();
    for (auto [entity, transform, mesh_comp] : view.each())
    {
        if (!mesh_comp.is_visible)
        {
            continue;
        }

        const uint64_t        mesh_key    = static_cast<uint64_t>(mesh_comp.mesh_id);
        SharedPtr<RenderMesh> cached_mesh = Renderer::get_resource_cache().find_mesh(mesh_key);
        if (!cached_mesh)
        {
            continue;
        }
        const RenderMesh* render_mesh = cached_mesh.get();
        const uint16_t    buffer_id   = Renderer::get_resource_cache().find_mesh_id(mesh_key);

        // Resolve material from cache only — no loading or compilation permitted here.
        const MaterialComponent* mat_comp = scene.registry().try_get<MaterialComponent>(entity);
        const uint64_t           mat_key  = mat_comp ? static_cast<uint64_t>(mat_comp->material_id) : 0;

        SharedPtr<Material> cached_mat = mat_key ? Renderer::get_resource_cache().find_material(mat_key) : nullptr;
        if (!cached_mat)
        {
            cached_mat = m_fallback_material;
        }
        if (!cached_mat)
        {
            continue;
        }

        const uint16_t material_id = (mat_key && mat_key != 0)
                                         ? Renderer::get_resource_cache().find_material_id(mat_key)
                                         : m_fallback_material_id;
        // Forward and depth PSOs are 1:1 with material in Phase 1.
        // XOR 0x8000 keeps depth_pso_id in disjoint bit-space from pso_id within each 16-bit slot.
        const uint16_t pso_id       = material_id;
        const uint16_t depth_pso_id = material_id ^ 0x8000u;

        const Math::Mat4f  world        = transform.to_mat4();
        const Math::Vec3f  world_center = (world * Math::Vec4f(render_mesh->get_bounds_center(), 1.0f)).xyz();
        const Math::Vec3f& s            = transform.scale;
        const float        world_radius = render_mesh->get_bounds_radius() * Math::max(s.x, Math::max(s.y, s.z));

        const float    raw_z     = cam_view.row(2).x * world_center.x + cam_view.row(2).y * world_center.y +
                                   cam_view.row(2).z * world_center.z + cam_view.row(2).w;
        const float    view_z    = Math::max(0.0f, Math::min(raw_z, k_depth_range));
        const uint32_t depth_u32 = static_cast<uint32_t>(view_z / k_depth_range * float(UINT32_MAX));

        // Depth pre-pass key: [Z:32][buffer_id:16][depth_pso_id:16] — ascending = front-to-back.
        const uint64_t depth_key = (uint64_t(depth_u32) << 32) | (uint64_t(buffer_id) << 16) | uint64_t(depth_pso_id);

        // Forward key.
        const uint64_t fwd_key = [&]() -> uint64_t
        {
            if (cached_mat->is_transparent())
            {
                // bit63=1 forces transparents after all opaques; ~Z = back-to-front on ascending sort.
                return (uint64_t(1) << 63) | (uint64_t(~depth_u32) & ((uint64_t(1) << 63) - 1u));
            }
            // Opaque: [0:1][buffer_id:15][pso_id:15][material_id:16][Z:17]
            const uint32_t z17 = depth_u32 >> 15;
            return (uint64_t(buffer_id & 0x7FFFu) << 48) | (uint64_t(pso_id & 0x7FFFu) << 33) |
                   (uint64_t(material_id) << 17) | uint64_t(z17 & 0x1FFFFu);
        }();

        CullProxy proxy;
        proxy.world_center = world_center;
        proxy.world_radius = world_radius;
        proxy.entity_index = static_cast<uint32_t>(m_pool.size());
        m_cull_proxies.push_back(proxy);

        VisibleItem item;
        item.world_matrix = world;
        item.mesh         = render_mesh;
        item.pso          = cached_mat->get_pipeline_state();
        item.depth_pso    = cached_mat->get_depth_pso();
        item.material     = cached_mat.get();
        item.buffer_id    = buffer_id;
        item.pso_id       = pso_id;
        item.depth_pso_id = depth_pso_id;
        item.material_id  = material_id;
        item.depth_key    = depth_key;
        item.fwd_key      = fwd_key;
        m_pool.push_back(item);
    }
}

void SceneRenderer::build_commands()
{
    m_depth_batches.clear();
    m_fwd_batches.clear();
    m_instance_data.clear();

    if (m_visible.empty())
    {
        return;
    }

    // --- Depth pre-pass (opaques only, front-to-back) ---
    std::sort(m_visible.begin(), m_visible.end(),
              [](const VisibleItem& a, const VisibleItem& b) { return a.depth_key < b.depth_key; });

    for (const VisibleItem& item : m_visible)
    {
        if (item.fwd_key >> 63)
        {
            continue; // transparent: skip depth pre-pass
        }
        if (m_instance_data.size() >= k_max_instances)
        {
            IG_CORE_ASSERT(false, "SceneRenderer: k_max_instances exceeded in depth pre-pass");
            break;
        }

        const bool same = !m_depth_batches.empty() && m_depth_batches.back().mesh == item.mesh &&
                          m_depth_batches.back().pso == item.depth_pso;
        if (!same)
        {
            const MeshSlot slot = item.mesh->get_mesh_slot();
            DrawBatch      batch;
            batch.args.index_count    = slot.index_count;
            batch.args.instance_count = 0;
            batch.args.first_index    = slot.first_index;
            batch.args.base_vertex    = slot.base_vertex;
            batch.args.base_instance  = static_cast<uint32_t>(m_instance_data.size());
            batch.mesh                = item.mesh;
            batch.pso                 = item.depth_pso;
            batch.material            = nullptr;
            batch.buffer_id           = item.buffer_id;
            batch.pso_id              = item.depth_pso_id;
            batch.material_id         = 0;
            m_depth_batches.push_back(batch);
        }
        m_depth_batches.back().args.instance_count++;

        GPUInstanceData gid;
        gid.world_matrix   = item.world_matrix;
        gid.material_index = 0;
        gid.padding[0] = gid.padding[1] = gid.padding[2] = 0;
        m_instance_data.push_back(gid);
    }

    // --- Forward pass (opaques state-minimised, then transparents back-to-front) ---
    std::sort(m_visible.begin(), m_visible.end(),
              [](const VisibleItem& a, const VisibleItem& b) { return a.fwd_key < b.fwd_key; });

    for (const VisibleItem& item : m_visible)
    {
        if (m_instance_data.size() >= k_max_instances)
        {
            IG_CORE_ASSERT(false, "SceneRenderer: k_max_instances exceeded in forward pass");
            break;
        }

        const bool transparent = (item.fwd_key >> 63) != 0;

        // Coalesce: same (buffer, pso, material, mesh_slot) → increment instance_count.
        // Transparents are never coalesced; draw order must be preserved exactly.
        const bool same = !transparent && !m_fwd_batches.empty() && m_fwd_batches.back().buffer_id == item.buffer_id &&
                          m_fwd_batches.back().pso_id == item.pso_id &&
                          m_fwd_batches.back().material_id == item.material_id &&
                          m_fwd_batches.back().mesh == item.mesh;
        if (!same)
        {
            const MeshSlot slot = item.mesh->get_mesh_slot();
            DrawBatch      batch;
            batch.args.index_count    = slot.index_count;
            batch.args.instance_count = 0;
            batch.args.first_index    = slot.first_index;
            batch.args.base_vertex    = slot.base_vertex;
            batch.args.base_instance  = static_cast<uint32_t>(m_instance_data.size());
            batch.mesh                = item.mesh;
            batch.pso                 = item.pso;
            batch.material            = item.material;
            batch.buffer_id           = item.buffer_id;
            batch.pso_id              = item.pso_id;
            batch.material_id         = item.material_id;
            m_fwd_batches.push_back(batch);
        }
        m_fwd_batches.back().args.instance_count++;

        GPUInstanceData gid;
        gid.world_matrix   = item.world_matrix;
        gid.material_index = 0;
        gid.padding[0] = gid.padding[1] = gid.padding[2] = 0;
        m_instance_data.push_back(gid);
    }

    // Upload instance data — Dynamic/Shared storage; no explicit flush on Apple Silicon.
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
    build_cull_proxies(scene, camera.view);

    m_visible.clear();
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_cull_proxies.size()); ++i)
    {
        const CullProxy& proxy = m_cull_proxies[i];
        if (Math::frustum_contains_sphere(camera.frustum, proxy.world_center, proxy.world_radius))
        {
            m_visible.push_back(m_pool[proxy.entity_index]);
        }
    }

    build_commands();

    RGTextureHandle depth = builder.import_viewport_depth();

    // --- Depth pre-pass ---
    builder.write_depth_stencil(depth, {GRILoadAction::Clear, GRIStoreAction::Store, 1.0f});
    builder.add_pass(
        "DepthPrePass",
        [this](GRICommandList& cmd)
        {
            if (m_depth_batches.empty() || !m_instance_buffer)
            {
                return;
            }
            Renderer::bind_frame_data(cmd);
            cmd.set_vertex_buffer(m_instance_buffer.get(), 0, k_instance_buffer_slot);

            GRIPipelineState* current_pso = nullptr;
            GRIBuffer*        current_vb  = nullptr;
            GRIBuffer*        global_vb   = StaticGeometryBatcher::get().get_global_vb();
            GRIBuffer*        global_ib   = StaticGeometryBatcher::get().get_global_ib();
            for (const DrawBatch& batch : m_depth_batches)
            {
                if (batch.pso != current_pso)
                {
                    cmd.set_graphics_pipeline_state(batch.pso);
                    current_pso = batch.pso;
                }
                GRIBuffer* desired_vb = batch.mesh->get_vertex_buffer() ? batch.mesh->get_vertex_buffer() : global_vb;
                if (desired_vb && desired_vb != current_vb)
                {
                    GRIBuffer* desired_ib = batch.mesh->get_index_buffer() ? batch.mesh->get_index_buffer() : global_ib;
                    cmd.set_vertex_buffer(desired_vb);
                    cmd.set_index_buffer(desired_ib, batch.mesh->get_index_buffer() ? batch.mesh->get_index_format()
                                                                                    : GRIIndexFormat::Uint32);
                    current_vb = desired_vb;
                }
                cmd.draw_indexed_primitives_instanced(batch.args.index_count, batch.args.instance_count,
                                                      batch.args.base_instance, batch.args.first_index,
                                                      batch.args.base_vertex);
            }
        });

    // Explicit dependency: DepthPrePass write must complete before ForwardScene reads.
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
            if (m_fwd_batches.empty() || !m_instance_buffer)
            {
                return;
            }
            Renderer::bind_frame_data(cmd);
            cmd.set_texture(p->scene_texture, 0, GRIShaderStage::Pixel);
            cmd.set_vertex_buffer(m_instance_buffer.get(), 0, k_instance_buffer_slot);

            GRIPipelineState* current_pso = nullptr;
            const Material*   current_mat = nullptr;
            GRIBuffer*        current_vb  = nullptr;
            GRIBuffer*        global_vb   = StaticGeometryBatcher::get().get_global_vb();
            GRIBuffer*        global_ib   = StaticGeometryBatcher::get().get_global_ib();
            for (const DrawBatch& batch : m_fwd_batches)
            {
                if (batch.pso != current_pso)
                {
                    cmd.set_graphics_pipeline_state(batch.pso);
                    current_pso = batch.pso;
                }
                if (batch.material != current_mat)
                {
                    if (batch.material->get_params_buffer())
                    {
                        cmd.set_uniform_buffer(batch.material->get_params_buffer(),
                                               static_cast<uint32_t>(UniformSlot::MaterialArgs), GRIShaderStage::Pixel);
                    }
                    current_mat = batch.material;
                }
                GRIBuffer* desired_vb = batch.mesh->get_vertex_buffer() ? batch.mesh->get_vertex_buffer() : global_vb;
                if (desired_vb && desired_vb != current_vb)
                {
                    GRIBuffer* desired_ib = batch.mesh->get_index_buffer() ? batch.mesh->get_index_buffer() : global_ib;
                    cmd.set_vertex_buffer(desired_vb);
                    cmd.set_index_buffer(desired_ib, batch.mesh->get_index_buffer() ? batch.mesh->get_index_format()
                                                                                    : GRIIndexFormat::Uint32);
                    current_vb = desired_vb;
                }
                cmd.draw_indexed_primitives_instanced(batch.args.index_count, batch.args.instance_count,
                                                      batch.args.base_instance, batch.args.first_index,
                                                      batch.args.base_vertex);
            }
        });
}

} // namespace Ignis
