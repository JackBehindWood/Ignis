#include "igpch.h"
#include "SceneExtractor.h"
#include "Components/Components.h"
#include "Ignis/Asset/AssetManager.h"
#include "Ignis/Asset/AssetMesh.h"
#include "Ignis/Asset/AssetMaterial.h"
#include "Ignis/Asset/AssetTexture2D.h"
#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/PBRMaterialParams.h"
#include "Ignis/Rendering/Renderer.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/RenderTexture2D.h"
#include "Ignis/Rendering/Shaders/ShaderCache.h"
#include "Ignis/Rendering/VertexDeclarationRegistry.h"
#include "Ignis/Rendering/StaticGeometryBatcher.h"
#include "Ignis/Math/Frustum.h"

namespace Ignis
{

static constexpr uint64_t k_inline_mat_bit = 0x8000'0000'0000'0000ULL;

void SceneExtractor::prepare(Scene& scene)
{
    AssetManager&        am        = AssetManager::get();
    const GRIPixelFormat rt_fmt    = GRIPixelFormat::RGBA16Float;
    const GRIPixelFormat depth_fmt = Renderer::get_config().depth_format;

    // Fallback material — created once; used for any entity whose material isn't ready.
    if (!Renderer::get_resource_cache().find_material(k_fallback_material_key))
    {
        SharedPtr<RenderShader> vs = Renderer::get_global_cache().get_pbr_vs();
        SharedPtr<RenderShader> ps = Renderer::get_global_cache().get_pbr_ps();
        if (vs && ps)
        {
            PBRMaterialParams defaults{};
            GRIBufferDesc     buf_desc;
            buf_desc.size           = sizeof(PBRMaterialParams);
            buf_desc.usage          = GRIBufferUsage::UniformBuffer | GRIBufferUsage::Dynamic;
            GRIBufferPtr params_buf = RenderSystem::get_gri()->create_buffer(buf_desc, &defaults);

            SharedPtr<Material> fallback = Renderer::get_material_factory().create_with_params(
                vs, ps, "standard_mesh", GRIPixelFormat::RGBA16Float, depth_fmt, {}, {}, {}, std::move(params_buf),
                defaults);
            Renderer::get_resource_cache().register_material(k_fallback_material_key, std::move(fallback));
        }
    }

    // White fallback texture — 1×1 RGBA8 all-white.
    if (!Renderer::get_resource_cache().find_texture(k_white_texture_key))
    {
        constexpr uint8_t k_white[4] = {0xFF, 0xFF, 0xFF, 0xFF};
        GRITexture2DDesc  desc;
        desc.width             = 1;
        desc.height            = 1;
        desc.format            = GRIPixelFormat::RGBA8Unorm;
        desc.initial_data      = k_white;
        desc.initial_data_size = 4;
        if (GRITexture2DPtr gri = RenderSystem::get_gri()->create_texture2d(desc))
        {
            auto rt = create_shared<RenderTexture2D>(std::move(gri), 1u, 1u, GRIPixelFormat::RGBA8Unorm);
            Renderer::get_resource_cache().register_texture(k_white_texture_key, std::move(rt));
        }
    }

    // Mesh registration.
    {
        auto view = scene.registry().view<MeshRendererComponent>();
        for (auto [entity, mesh_comp] : view.each())
        {
            const uint64_t key = static_cast<uint64_t>(mesh_comp.mesh_id);
            if (StaticGeometryBatcher::get().has_mesh(key))
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

    // Texture upload from TextureComponent.
    {
        auto view = scene.registry().view<TextureComponent>();
        for (auto [entity, tex_comp] : view.each())
        {
            const uint64_t key = static_cast<uint64_t>(tex_comp.texture_id);
            if (!key || Renderer::get_resource_cache().find_texture(key))
            {
                continue;
            }

            SharedPtr<AssetTexture2D> asset_tex = am.get_asset_as<AssetTexture2D>(tex_comp.texture_id);
            if (!asset_tex)
            {
                if (am.get_state(tex_comp.texture_id) == AssetState::Unloaded)
                {
                    am.load_deferred(tex_comp.texture_id);
                }
                continue;
            }

            GRITexture2DDesc desc;
            desc.width             = asset_tex->get_width();
            desc.height            = asset_tex->get_height();
            desc.format            = static_cast<GRIPixelFormat>(asset_tex->get_format());
            desc.initial_data      = asset_tex->get_pixels().data();
            desc.initial_data_size = static_cast<uint32_t>(asset_tex->get_pixels().size());
            if (GRITexture2DPtr gri = RenderSystem::get_gri()->create_texture2d(desc))
            {
                auto rt = create_shared<RenderTexture2D>(std::move(gri), desc.width, desc.height, desc.format);
                Renderer::get_resource_cache().register_texture(key, std::move(rt));
            }
        }
    }

    // Material and PSO compilation — file-backed and inline PBR paths.
    {
        const GlobalEngineCache& gec = Renderer::get_global_cache();

        auto upload_tex = [&](AssetID id) -> SharedPtr<RenderTexture2D>
        {
            if (!id)
            {
                return nullptr;
            }
            const uint64_t key = static_cast<uint64_t>(id);
            if (auto rt = Renderer::get_resource_cache().find_texture(key))
            {
                return rt;
            }
            SharedPtr<AssetTexture2D> asset_tex = am.get_asset_as<AssetTexture2D>(id);
            if (!asset_tex)
            {
                if (am.get_state(id) == AssetState::Unloaded)
                {
                    am.load_deferred(id);
                }
                return nullptr;
            }
            GRITexture2DDesc desc;
            desc.width             = asset_tex->get_width();
            desc.height            = asset_tex->get_height();
            desc.format            = static_cast<GRIPixelFormat>(asset_tex->get_format());
            desc.initial_data      = asset_tex->get_pixels().data();
            desc.initial_data_size = static_cast<uint32_t>(asset_tex->get_pixels().size());
            if (GRITexture2DPtr gri = RenderSystem::get_gri()->create_texture2d(desc))
            {
                auto rt = create_shared<RenderTexture2D>(std::move(gri), desc.width, desc.height, desc.format);
                Renderer::get_resource_cache().register_texture(key, rt);
                return rt;
            }
            return nullptr;
        };

        auto resolve_bindless = [&](AssetID id, uint32_t fallback) -> uint32_t
        {
            if (!id)
            {
                return fallback;
            }
            auto rt = Renderer::get_resource_cache().find_texture(static_cast<uint64_t>(id));
            if (!rt)
            {
                return fallback;
            }
            GRISamplerDesc sd;
            sd.linear_filter        = true;
            GRISamplerStatePtr samp = RenderSystem::get_gri()->create_sampler_state(sd);
            return RenderSystem::get_gri()->register_bindless_texture(rt->get_texture_ptr(), std::move(samp));
        };

        auto view = scene.registry().view<MaterialComponent, IDComponent>();
        for (auto [entity, mat_comp, id_comp] : view.each())
        {
            if (mat_comp.material_id)
            {
                // Branch A: file-backed .mat asset.
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

                SharedPtr<RenderShader> vs = ShaderCache::get().get_or_compile_async(asset_mat->get_shader_source(),
                                                                                     GRIShaderStage::Vertex, opts);
                SharedPtr<RenderShader> ps = ShaderCache::get().get_or_compile_async(asset_mat->get_shader_source(),
                                                                                     GRIShaderStage::Pixel, opts);
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
                continue;
            }

            // Branch B: inline PBR — keyed by entity UUID with high-bit tag.
            const uint64_t key = k_inline_mat_bit | static_cast<uint64_t>(id_comp.id);

            upload_tex(mat_comp.albedo_tex);
            upload_tex(mat_comp.normal_tex);
            upload_tex(mat_comp.roughness_tex);
            upload_tex(mat_comp.metallic_tex);
            upload_tex(mat_comp.ao_tex);
            upload_tex(mat_comp.emissive_tex);

            SharedPtr<Material> existing = Renderer::get_resource_cache().find_material(key);
            if (existing)
            {
                if (mat_comp.params_dirty)
                {
                    PBRMaterialParams p  = existing->get_pbr_params();
                    p.albedo_colour      = mat_comp.albedo_colour;
                    p.emissive_colour    = mat_comp.emissive_colour;
                    p.alpha_cutoff       = mat_comp.alpha_cutoff;
                    p.emissive_intensity = mat_comp.emissive_intensity;
                    existing->update_pbr_params(p);
                    mat_comp.params_dirty = false;
                }
                continue;
            }

            SharedPtr<RenderShader> vs = gec.get_pbr_vs();
            SharedPtr<RenderShader> ps = gec.get_pbr_ps();
            if (!vs || !ps)
            {
                continue;
            }

            PBRMaterialParams p{};
            p.albedo_colour      = mat_comp.albedo_colour;
            p.emissive_colour    = mat_comp.emissive_colour;
            p.alpha_cutoff       = mat_comp.alpha_cutoff;
            p.emissive_intensity = mat_comp.emissive_intensity;
            p.albedo_tex         = resolve_bindless(mat_comp.albedo_tex, gec.get_default_white_idx());
            p.normal_tex         = resolve_bindless(mat_comp.normal_tex, gec.get_default_normal_idx());
            p.roughness_tex      = resolve_bindless(mat_comp.roughness_tex, gec.get_default_gray_idx());
            p.metallic_tex       = resolve_bindless(mat_comp.metallic_tex, gec.get_default_black_idx());
            p.ao_tex             = resolve_bindless(mat_comp.ao_tex, gec.get_default_white_idx());
            p.emissive_tex       = resolve_bindless(mat_comp.emissive_tex, gec.get_default_black_idx());

            GRIBufferDesc bd;
            bd.size          = sizeof(PBRMaterialParams);
            bd.usage         = GRIBufferUsage::UniformBuffer | GRIBufferUsage::Dynamic;
            GRIBufferPtr buf = RenderSystem::get_gri()->create_buffer(bd, &p);

            SharedPtr<Material> mat = Renderer::get_material_factory().create_with_params(
                vs, ps, "standard_mesh", rt_fmt, depth_fmt, {}, {}, {}, std::move(buf), p);
            Renderer::get_resource_cache().register_material(key, std::move(mat));
            mat_comp.params_dirty = false;
        }
    }
}

const RenderScene& SceneExtractor::extract(const Scene& scene, const CameraData& camera)
{
    m_scene.instance_data.clear();
    m_scene.cull_proxies.clear();
    m_vis_scratch.clear();

    RenderResourceCache& cache = Renderer::get_resource_cache();

    // Traverse entities and build DrawProxy candidates.
    auto view = scene.registry().view<TransformComponent, MeshRendererComponent>();
    for (auto [entity, transform, mesh_comp] : view.each())
    {
        if (!mesh_comp.is_visible)
        {
            continue;
        }
        if (m_vis_scratch.size() >= k_max_instances)
        {
            IG_CORE_ASSERT(false, "SceneExtractor: k_max_instances exceeded");
            break;
        }

        const uint64_t        mesh_key    = static_cast<uint64_t>(mesh_comp.mesh_id);
        SharedPtr<RenderMesh> cached_mesh = cache.find_mesh(mesh_key);
        if (!cached_mesh)
        {
            continue;
        }

        const uint16_t buffer_id = cache.find_mesh_id(mesh_key);

        const MaterialComponent* mat_comp = scene.registry().try_get<MaterialComponent>(entity);
        const IDComponent*       id_comp  = scene.registry().try_get<IDComponent>(entity);
        uint64_t                 mat_key  = 0;
        if (mat_comp)
        {
            mat_key = mat_comp->material_id ? static_cast<uint64_t>(mat_comp->material_id)
                                            : (id_comp ? (k_inline_mat_bit | static_cast<uint64_t>(id_comp->id)) : 0);
        }

        SharedPtr<Material> cached_mat   = mat_key ? cache.find_material(mat_key) : nullptr;
        const uint64_t      used_mat_key = cached_mat ? mat_key : k_fallback_material_key;
        if (!cached_mat)
        {
            cached_mat = cache.find_material(k_fallback_material_key);
        }
        if (!cached_mat)
        {
            continue;
        }

        const uint16_t material_id  = cache.find_material_id(used_mat_key);
        const uint16_t depth_pso_id = material_id ^ 0x8000u;

        const Math::Mat4f  world        = transform.to_mat4();
        const Math::Vec3f  world_center = (world * Math::Vec4f(cached_mesh->get_bounds_center(), 1.0f)).xyz();
        const Math::Vec3f& s            = transform.scale;
        const float        world_radius = cached_mesh->get_bounds_radius() * Math::max(s.x, Math::max(s.y, s.z));

        const float    raw_z     = camera.view.row(2).x * world_center.x + camera.view.row(2).y * world_center.y +
                                   camera.view.row(2).z * world_center.z + camera.view.row(2).w;
        const float    view_z    = Math::max(0.0f, Math::min(raw_z, k_depth_range));
        const uint32_t depth_u32 = static_cast<uint32_t>(view_z / k_depth_range * float(UINT32_MAX));

        const uint64_t depth_key = (uint64_t(depth_u32) << 32) | (uint64_t(buffer_id) << 16) | uint64_t(depth_pso_id);

        const bool     transparent = cached_mat->is_transparent();
        const uint64_t fwd_key     = [&]() -> uint64_t
        {
            if (transparent)
            {
                return (uint64_t(1) << 63) | (uint64_t(~depth_u32) & ((uint64_t(1) << 63) - 1u));
            }
            const uint32_t z17 = depth_u32 >> 15;
            return (uint64_t(buffer_id & 0x7FFFu) << 48) | (uint64_t(material_id & 0x7FFFu) << 33) |
                   (uint64_t(material_id) << 17) | uint64_t(z17 & 0x1FFFFu);
        }();

        const TextureComponent* tex_comp     = scene.registry().try_get<TextureComponent>(entity);
        const uint64_t          tex_key      = tex_comp ? static_cast<uint64_t>(tex_comp->texture_id) : 0;
        const uint64_t          used_tex_key = (tex_key && cache.find_texture(tex_key)) ? tex_key : 0;

        const uint32_t entity_index = static_cast<uint32_t>(m_scene.instance_data.size());

        GPUInstanceData gid;
        gid.world_matrix   = world;
        gid.material_index = 0;
        gid.padding[0] = gid.padding[1] = gid.padding[2] = 0;
        m_scene.instance_data.push_back(gid);

        CullProxy cull;
        cull.world_center = world_center;
        cull.world_radius = world_radius;
        cull.entity_index = entity_index;
        m_scene.cull_proxies.push_back(cull);

        DrawProxy dp;
        dp.slot           = cached_mesh->get_mesh_slot();
        dp.entity_index   = entity_index;
        dp.buffer_id      = buffer_id;
        dp.material_id    = material_id;
        dp.depth_pso_id   = depth_pso_id;
        dp.is_transparent = transparent;
        dp.sort_key_depth = depth_key;
        dp.sort_key_fwd   = fwd_key;
        dp.mat_key        = used_mat_key;
        dp.texture_key    = used_tex_key;
        m_vis_scratch.push_back(dp);
    }

    // CPU frustum cull — split visible candidates into depth (opaques) and fwd lists.
    m_scene.depth_proxies.clear();
    m_scene.fwd_proxies.clear();
    m_scene.frustum = camera.frustum;

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_scene.cull_proxies.size()); ++i)
    {
        const CullProxy& p = m_scene.cull_proxies[i];
        if (!Math::frustum_contains_sphere(camera.frustum, p.world_center, p.world_radius))
        {
            continue;
        }
        const DrawProxy& dp = m_vis_scratch[i];
        if (!dp.is_transparent)
        {
            m_scene.depth_proxies.push_back(dp);
        }
        m_scene.fwd_proxies.push_back(dp);
    }

    // Sort depth pre-pass: ascending depth_key (front-to-back).
    std::sort(m_scene.depth_proxies.begin(), m_scene.depth_proxies.end(),
              [](const DrawProxy& a, const DrawProxy& b) { return a.sort_key_depth < b.sort_key_depth; });

    // Sort forward pass: ascending fwd_key (opaque state-min, transparents back-to-front).
    std::sort(m_scene.fwd_proxies.begin(), m_scene.fwd_proxies.end(),
              [](const DrawProxy& a, const DrawProxy& b) { return a.sort_key_fwd < b.sort_key_fwd; });

    // Gather lights.
    GPUFrameData fd{};
    fd.view_projection  = camera.view_projection;
    fd.camera_world_pos = camera.position;

    auto dir_view = scene.registry().view<TransformComponent, DirectionalLightComponent>();
    for (auto [e, tc, lc] : dir_view.each())
    {
        if (fd.num_directional_lights >= k_max_directional_lights)
        {
            break;
        }
        GPUDirectionalLight& gl = fd.directional_lights[fd.num_directional_lights++];
        gl.direction            = lc.direction;
        Math::normalize(gl.direction);
        gl.intensity = lc.intensity;
        gl.color     = lc.color;
    }

    auto pt_view = scene.registry().view<TransformComponent, PointLightComponent>();
    for (auto [e, tc, lc] : pt_view.each())
    {
        if (fd.num_point_lights >= k_max_point_lights)
        {
            break;
        }
        GPUPointLight& gl = fd.point_lights[fd.num_point_lights++];
        gl.position       = tc.position;
        gl.radius         = lc.radius;
        gl.color          = lc.color;
        gl.intensity      = lc.intensity;
    }

    m_scene.frame_data = fd;

    return m_scene;
}

} // namespace Ignis
