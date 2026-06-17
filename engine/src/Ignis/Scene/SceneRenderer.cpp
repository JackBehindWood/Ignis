#include "igpch.h"
#include "SceneRenderer.h"
#include "Ignis/Rendering/Renderer.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/RenderTexture2D.h"
#include "Ignis/Rendering/RenderGraph/RGBuilder.h"
#include "Ignis/Rendering/StaticGeometryBatcher.h"

namespace Ignis
{

void SceneRenderer::ensure_gpu_buffers()
{
    if (!m_entity_data_buffer)
    {
        GRIBufferDesc d;
        d.size               = k_max_instances * static_cast<uint32_t>(sizeof(GPUInstanceData));
        d.usage              = static_cast<GRIBufferUsage>(static_cast<uint32_t>(GRIBufferUsage::VertexBuffer) |
                                                           static_cast<uint32_t>(GRIBufferUsage::Dynamic));
        m_entity_data_buffer = RenderSystem::get_gri()->create_buffer(d);
    }
    if (!m_cull_input_buffer)
    {
        GRIBufferDesc d;
        d.size              = k_max_instances * static_cast<uint32_t>(sizeof(GPUCullInstance));
        d.usage             = GRIBufferUsage::StorageBuffer;
        m_cull_input_buffer = RenderSystem::get_gri()->create_buffer(d);
    }
    if (!m_visible_indices_buffer)
    {
        GRIBufferDesc d;
        d.size                   = k_max_instances * 2u * static_cast<uint32_t>(sizeof(uint32_t));
        d.usage                  = static_cast<GRIBufferUsage>(static_cast<uint32_t>(GRIBufferUsage::StorageBuffer) |
                                                               static_cast<uint32_t>(GRIBufferUsage::Dynamic));
        m_visible_indices_buffer = RenderSystem::get_gri()->create_buffer(d);
    }
    if (!m_gpu_cull_visible_buffer)
    {
        GRIBufferDesc d;
        d.size                    = k_max_instances * 2u * static_cast<uint32_t>(sizeof(uint32_t));
        d.usage                   = static_cast<GRIBufferUsage>(static_cast<uint32_t>(GRIBufferUsage::StorageBuffer) |
                                                                static_cast<uint32_t>(GRIBufferUsage::Dynamic));
        m_gpu_cull_visible_buffer = RenderSystem::get_gri()->create_buffer(d);
    }
    if (!m_atomic_counter_buffer)
    {
        GRIBufferDesc d;
        d.size                  = static_cast<uint32_t>(sizeof(uint32_t));
        d.usage                 = static_cast<GRIBufferUsage>(static_cast<uint32_t>(GRIBufferUsage::StorageBuffer) |
                                                              static_cast<uint32_t>(GRIBufferUsage::Dynamic));
        m_atomic_counter_buffer = RenderSystem::get_gri()->create_buffer(d);
    }
    if (!m_draw_args_buffer)
    {
        GRIBufferDesc d;
        d.size             = k_max_batches * static_cast<uint32_t>(sizeof(DrawIndexedArguments));
        d.usage            = static_cast<GRIBufferUsage>(static_cast<uint32_t>(GRIBufferUsage::StorageBuffer) |
                                                         static_cast<uint32_t>(GRIBufferUsage::IndirectBuffer) |
                                                         static_cast<uint32_t>(GRIBufferUsage::Dynamic));
        m_draw_args_buffer = RenderSystem::get_gri()->create_buffer(d);
    }
    if (!m_cull_cb)
    {
        GRIBufferDesc d;
        d.size    = static_cast<uint32_t>(sizeof(GPUCullConstants));
        d.usage   = static_cast<GRIBufferUsage>(static_cast<uint32_t>(GRIBufferUsage::UniformBuffer) |
                                                static_cast<uint32_t>(GRIBufferUsage::Dynamic));
        m_cull_cb = RenderSystem::get_gri()->create_buffer(d);
    }
}

void SceneRenderer::build_commands(const RenderScene& rs)
{
    m_depth_batches.clear();
    m_fwd_batches.clear();

    if (rs.depth_proxies.empty() && rs.fwd_proxies.empty())
    {
        return;
    }

    RenderResourceCache& cache = Renderer::get_resource_cache();

    Vector<uint32_t> cpu_visible_indices;
    cpu_visible_indices.reserve(rs.depth_proxies.size() + rs.fwd_proxies.size());
    uint32_t total_visible = 0;

    // --- Depth pre-pass (opaques, already sorted front-to-back) ---
    for (const DrawProxy& proxy : rs.depth_proxies)
    {
        if (total_visible >= k_max_instances)
        {
            IG_CORE_ASSERT(false, "SceneRenderer: k_max_instances exceeded in depth pre-pass");
            break;
        }

        SharedPtr<Material> mat = cache.find_material(proxy.mat_key);
        if (!mat)
        {
            continue;
        }

        GRIPipelineState* depth_pso = mat->get_depth_pso();

        const bool merge = !m_depth_batches.empty() && m_depth_batches.back().pso_id == proxy.depth_pso_id &&
                           m_depth_batches.back().buffer_id == proxy.buffer_id;
        if (!merge)
        {
            DrawBatch batch;
            batch.args.index_count    = proxy.slot.index_count;
            batch.args.instance_count = 0;
            batch.args.first_index    = proxy.slot.first_index;
            batch.args.base_vertex    = proxy.slot.base_vertex;
            batch.args.base_instance  = total_visible;
            batch.mesh                = nullptr; // global batcher mesh — resolved at draw time
            batch.pso                 = depth_pso;
            batch.material            = nullptr;
            batch.buffer_id           = proxy.buffer_id;
            batch.pso_id              = proxy.depth_pso_id;
            batch.material_id         = 0;
            m_depth_batches.push_back(batch);
        }
        m_depth_batches.back().args.instance_count++;
        cpu_visible_indices.push_back(proxy.entity_index);
        total_visible++;
    }

    // --- Forward pass (already sorted by fwd_key) ---
    for (const DrawProxy& proxy : rs.fwd_proxies)
    {
        if (total_visible >= k_max_instances)
        {
            IG_CORE_ASSERT(false, "SceneRenderer: k_max_instances exceeded in forward pass");
            break;
        }

        SharedPtr<Material> mat = cache.find_material(proxy.mat_key);
        if (!mat)
        {
            continue;
        }

        GRITexture2D* tex = nullptr;
        if (proxy.texture_key)
        {
            if (SharedPtr<RenderTexture2D> rt = cache.find_texture(proxy.texture_key))
            {
                tex = rt->get_texture();
            }
        }
        if (!tex)
        {
            if (SharedPtr<RenderTexture2D> wt = cache.find_texture(k_white_texture_key))
            {
                tex = wt->get_texture();
            }
        }

        const bool transparent = proxy.is_transparent;
        const bool merge = !transparent && !m_fwd_batches.empty() &&
                           m_fwd_batches.back().buffer_id == proxy.buffer_id &&
                           m_fwd_batches.back().pso_id == proxy.material_id &&
                           m_fwd_batches.back().material_id == proxy.material_id && m_fwd_batches.back().texture == tex;
        if (!merge)
        {
            DrawBatch batch;
            batch.args.index_count    = proxy.slot.index_count;
            batch.args.instance_count = 0;
            batch.args.first_index    = proxy.slot.first_index;
            batch.args.base_vertex    = proxy.slot.base_vertex;
            batch.args.base_instance  = total_visible;
            batch.mesh                = nullptr;
            batch.pso                 = mat->get_pipeline_state();
            batch.material            = mat.get();
            batch.texture             = tex;
            batch.buffer_id           = proxy.buffer_id;
            batch.pso_id              = proxy.material_id;
            batch.material_id         = proxy.material_id;
            m_fwd_batches.push_back(batch);
        }
        m_fwd_batches.back().args.instance_count++;
        cpu_visible_indices.push_back(proxy.entity_index);
        total_visible++;
    }

    if (cpu_visible_indices.empty())
    {
        return;
    }

    IG_CORE_ASSERT(m_visible_indices_buffer && m_draw_args_buffer, "SceneRenderer: GPU buffers not allocated");

    RenderSystem::get_gri()->update_buffer(m_visible_indices_buffer.get(), cpu_visible_indices.data(),
                                           static_cast<uint32_t>(cpu_visible_indices.size() * sizeof(uint32_t)));

    const uint32_t total_batches = static_cast<uint32_t>(m_depth_batches.size() + m_fwd_batches.size());
    IG_CORE_ASSERT(total_batches <= k_max_batches, "SceneRenderer: k_max_batches exceeded");

    Vector<DrawIndexedArguments> all_args;
    all_args.reserve(total_batches);
    for (const DrawBatch& b : m_depth_batches)
    {
        all_args.push_back(b.args);
    }
    for (const DrawBatch& b : m_fwd_batches)
    {
        all_args.push_back(b.args);
    }

    RenderSystem::get_gri()->update_buffer(m_draw_args_buffer.get(), all_args.data(),
                                           static_cast<uint32_t>(total_batches * sizeof(DrawIndexedArguments)));
}

void SceneRenderer::resize(uint32_t w, uint32_t h)
{
    if (w == 0 || h == 0 || (w == m_rt_width && h == m_rt_height))
    {
        return;
    }

    m_rt_width  = w;
    m_rt_height = h;

    GRITexture2DDesc color_desc;
    color_desc.width          = w;
    color_desc.height         = h;
    color_desc.num_mip_levels = 1;
    color_desc.format         = GRIPixelFormat::RGBA16Float;
    m_color_rt                = RenderSystem::get_gri()->create_texture2d(color_desc);

    GRITexture2DDesc ldr_desc;
    ldr_desc.width          = w;
    ldr_desc.height         = h;
    ldr_desc.num_mip_levels = 1;
    ldr_desc.format         = GRIPixelFormat::BGRA8Unorm;
    m_ldr_rt                = RenderSystem::get_gri()->create_texture2d(ldr_desc);

    GRITexture2DDesc depth_desc;
    depth_desc.width          = w;
    depth_desc.height         = h;
    depth_desc.num_mip_levels = 1;
    depth_desc.format         = GRIPixelFormat::Depth32Float;
    m_depth_rt                = RenderSystem::get_gri()->create_texture2d(depth_desc);
}

SceneRenderHandles SceneRenderer::render_scene(const RenderScene& rs, RGBuilder& builder)
{
    IG_ASSERT(m_color_rt && m_depth_rt && m_ldr_rt,
              "SceneRenderer: RTs not initialized — call resize() before render_scene()");

    GPUFrameData fd = rs.frame_data;
    fd.debug_mode   = m_debug_mode;
    Renderer::upload_frame_data(fd);

    // Upload instance data and cull proxies to persistent GPU buffers.
    const uint32_t entity_count = static_cast<uint32_t>(rs.instance_data.size());
    if (entity_count > 0)
    {
        ensure_gpu_buffers();
        IG_CORE_ASSERT(entity_count <= k_max_instances, "SceneRenderer: entity_count exceeds k_max_instances");

        for (uint32_t i = 0; i < entity_count; ++i)
        {
            RenderSystem::get_gri()->update_buffer(m_entity_data_buffer.get(), &rs.instance_data[i],
                                                   static_cast<uint32_t>(sizeof(GPUInstanceData)),
                                                   i * static_cast<uint32_t>(sizeof(GPUInstanceData)));
        }

        for (uint32_t i = 0; i < entity_count; ++i)
        {
            const CullProxy& proxy = rs.cull_proxies[i];
            GPUCullInstance  ci;
            ci.world_center = proxy.world_center;
            ci.world_radius = proxy.world_radius;
            ci.entity_index = i;
            ci._pad[0] = ci._pad[1] = ci._pad[2] = 0;
            RenderSystem::get_gri()->update_buffer(m_cull_input_buffer.get(), &ci,
                                                   static_cast<uint32_t>(sizeof(GPUCullInstance)),
                                                   i * static_cast<uint32_t>(sizeof(GPUCullInstance)));
        }

        GPUCullConstants cc;
        for (int32_t i = 0; i < 6; ++i)
        {
            cc.planes[i].normal = Math::Vec3f(rs.frustum.planes[i].x, rs.frustum.planes[i].y, rs.frustum.planes[i].z);
            cc.planes[i].d      = rs.frustum.planes[i].w;
        }
        cc.instance_count = entity_count;
        cc._pad[0] = cc._pad[1] = cc._pad[2] = 0;
        RenderSystem::get_gri()->update_buffer(m_cull_cb.get(), &cc, sizeof(GPUCullConstants));

        const uint32_t zero = 0;
        RenderSystem::get_gri()->update_buffer(m_atomic_counter_buffer.get(), &zero, sizeof(uint32_t));
    }

    build_commands(rs);

    RGTextureHandle color = builder.import_texture("scene_color", m_color_rt.get());
    RGTextureHandle depth = builder.import_texture("scene_depth", m_depth_rt.get());

    const bool has_draws = (!m_depth_batches.empty() || !m_fwd_batches.empty()) && m_entity_data_buffer &&
                           m_draw_args_buffer && m_visible_indices_buffer;

    constexpr GRIBufferUsage k_storage_dynamic = static_cast<GRIBufferUsage>(
        static_cast<uint32_t>(GRIBufferUsage::StorageBuffer) | static_cast<uint32_t>(GRIBufferUsage::Dynamic));
    constexpr GRIBufferUsage k_indirect_dynamic = static_cast<GRIBufferUsage>(
        static_cast<uint32_t>(GRIBufferUsage::StorageBuffer) | static_cast<uint32_t>(GRIBufferUsage::IndirectBuffer) |
        static_cast<uint32_t>(GRIBufferUsage::Dynamic));

    RGBufferHandle vis_h = has_draws
                               ? builder.import_buffer("vis_indices", m_visible_indices_buffer.get(), k_storage_dynamic)
                               : RGBufferHandle{};
    RGBufferHandle draw_args_h =
        has_draws ? builder.import_buffer("draw_args", m_draw_args_buffer.get(), k_indirect_dynamic) : RGBufferHandle{};

    GRIComputePipelineStatePtr cull_pso = Renderer::get_global_cache().get_cull_pipeline_state();

    RGBufferHandle gpu_vis_h;
    if (cull_pso && entity_count > 0 && m_cull_input_buffer && m_gpu_cull_visible_buffer)
    {
        RGBufferHandle cull_input_h =
            builder.import_buffer("cull_input", m_cull_input_buffer.get(), GRIBufferUsage::StorageBuffer);
        RGBufferHandle counter_h =
            builder.import_buffer("cull_counter", m_atomic_counter_buffer.get(), k_storage_dynamic);
        gpu_vis_h = builder.import_buffer("gpu_cull_vis", m_gpu_cull_visible_buffer.get(), k_storage_dynamic);

        builder.read_storage_buffer(cull_input_h);
        builder.write_storage_buffer(gpu_vis_h);
        builder.write_storage_buffer(counter_h);
        builder.add_compute_pass("GPUCull",
                                 [this, cull_pso, entity_count](GRICommandList& cmd)
                                 {
                                     cmd.set_compute_pipeline_state(cull_pso.get());
                                     cmd.set_uniform_buffer(m_cull_cb.get(), 4, GRIShaderStage::Compute);
                                     cmd.set_storage_buffer(m_cull_input_buffer.get(), 5);
                                     cmd.set_storage_buffer(m_gpu_cull_visible_buffer.get(), 6);
                                     cmd.set_storage_buffer(m_atomic_counter_buffer.get(), 7);
                                     cmd.dispatch((entity_count + 63u) / 64u, 1, 1);
                                 });
    }

    // --- Depth pre-pass ---
    if (gpu_vis_h.is_valid())
    {
        builder.read_storage_buffer(gpu_vis_h);
    }
    if (vis_h.is_valid())
    {
        builder.read_storage_buffer(vis_h);
    }
    if (draw_args_h.is_valid())
    {
        builder.read_buffer(draw_args_h);
    }
    builder.write_depth_stencil(depth, {GRILoadAction::Clear, GRIStoreAction::Store, 1.0f});
    builder.add_pass("DepthPrePass", RGPassType::Graphics,
                     [this](GRICommandList& cmd)
                     {
                         if (m_depth_batches.empty() || !m_entity_data_buffer || !m_visible_indices_buffer)
                         {
                             return;
                         }
                         Renderer::bind_frame_data(cmd);
                         cmd.set_vertex_buffer(m_entity_data_buffer.get(), 0, k_entity_buffer_slot);
                         cmd.set_vertex_buffer(m_visible_indices_buffer.get(), 0, k_instance_buffer_slot);

                         GRIPipelineState* current_pso = nullptr;
                         GRIBuffer*        global_vb   = StaticGeometryBatcher::get().get_global_vb();
                         GRIBuffer*        global_ib   = StaticGeometryBatcher::get().get_global_ib();
                         if (!global_vb || !global_ib)
                         {
                             return;
                         }
                         cmd.set_vertex_buffer(global_vb);
                         cmd.set_index_buffer(global_ib, GRIIndexFormat::Uint32);
                         for (uint32_t i = 0; i < static_cast<uint32_t>(m_depth_batches.size()); ++i)
                         {
                             const DrawBatch& batch = m_depth_batches[i];
                             if (batch.pso != current_pso)
                             {
                                 cmd.set_graphics_pipeline_state(batch.pso);
                                 current_pso = batch.pso;
                             }
                             cmd.draw_indexed_primitives_indirect(
                                 m_draw_args_buffer.get(), i * static_cast<uint32_t>(sizeof(DrawIndexedArguments)));
                         }
                     });

    builder.read_texture(depth);

    // --- Forward scene pass ---
    if (vis_h.is_valid())
    {
        builder.read_storage_buffer(vis_h);
    }
    if (draw_args_h.is_valid())
    {
        builder.read_buffer(draw_args_h);
    }
    builder.write_render_target(0, color, RGColorAttachmentDesc::clear({0.0f, 0.0f, 0.0f, 1.0f}));
    builder.write_depth_stencil(depth, {GRILoadAction::Load, GRIStoreAction::DontCare, 1.0f});
    builder.add_pass("ForwardScene", RGPassType::Graphics,
                     [this](GRICommandList& cmd)
                     {
                         if (m_fwd_batches.empty() || !m_entity_data_buffer || !m_visible_indices_buffer)
                         {
                             return;
                         }
                         Renderer::bind_frame_data(cmd);

                         if (auto ibl = Renderer::get_global_cache().get_irradiance_cube())
                         {
                             cmd.set_texture(ibl->get_texture(), 0, GRIShaderStage::Pixel);
                         }
                         if (auto pre = Renderer::get_global_cache().get_prefilter_cube())
                         {
                             cmd.set_texture(pre->get_texture(), 1, GRIShaderStage::Pixel);
                         }
                         if (auto lut = Renderer::get_global_cache().get_brdf_lut())
                         {
                             cmd.set_texture(lut->get_texture(), 2, GRIShaderStage::Pixel);
                         }

                         cmd.set_vertex_buffer(m_entity_data_buffer.get(), 0, k_entity_buffer_slot);
                         cmd.set_vertex_buffer(m_visible_indices_buffer.get(), 0, k_instance_buffer_slot);

                         GRIPipelineState* current_pso = nullptr;
                         const Material*   current_mat = nullptr;
                         GRITexture2D*     current_tex = nullptr;
                         GRIBuffer*        global_vb   = StaticGeometryBatcher::get().get_global_vb();
                         GRIBuffer*        global_ib   = StaticGeometryBatcher::get().get_global_ib();
                         if (!global_vb || !global_ib)
                         {
                             return;
                         }
                         cmd.set_vertex_buffer(global_vb);
                         cmd.set_index_buffer(global_ib, GRIIndexFormat::Uint32);
                         const uint32_t depth_count = static_cast<uint32_t>(m_depth_batches.size());
                         for (uint32_t i = 0; i < static_cast<uint32_t>(m_fwd_batches.size()); ++i)
                         {
                             const DrawBatch& batch = m_fwd_batches[i];
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
                                                            static_cast<uint32_t>(DefaultBindings::MaterialArgs),
                                                            GRIShaderStage::Pixel);
                                 }
                                 current_mat = batch.material;
                             }
                             if (batch.texture != current_tex)
                             {
                                 cmd.set_texture(batch.texture, 0, GRIShaderStage::Pixel);
                                 current_tex = batch.texture;
                             }
                             cmd.draw_indexed_primitives_indirect(
                                 m_draw_args_buffer.get(),
                                 (depth_count + i) * static_cast<uint32_t>(sizeof(DrawIndexedArguments)));
                         }
                     });

    return {color, depth};
}

RGTextureHandle SceneRenderer::render_tonemap(RGTextureHandle hdr_color, RGBuilder& builder)
{
    SharedPtr<Material> tonemap_mat = Renderer::get_global_cache().get_tonemap_material();
    RGTextureHandle     ldr         = builder.import_texture("scene_ldr", m_ldr_rt.get());
    builder.read_texture(hdr_color);
    builder.write_render_target(0, ldr, RGColorAttachmentDesc::clear({0.0f, 0.0f, 0.0f, 1.0f}));
    builder.add_pass("Tonemap", RGPassType::Graphics,
                     [this, tonemap_mat](GRICommandList& cmd)
                     {
                         if (!tonemap_mat)
                         {
                             return;
                         }
                         cmd.set_graphics_pipeline_state(tonemap_mat->get_pipeline_state());
                         cmd.set_texture(m_color_rt.get(), 0, GRIShaderStage::Pixel);
                         cmd.draw_primitives(3);
                     });
    return ldr;
}

} // namespace Ignis
