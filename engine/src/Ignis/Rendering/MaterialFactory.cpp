#include "igpch.h"
#include "MaterialFactory.h"

#include "Ignis/Rendering/PBRMaterialParams.h"
#include "Ignis/Rendering/VertexDeclarationRegistry.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/Renderer.h"
#include "Ignis/Rendering/GRI/GRI.h"
#include "Ignis/Rendering/Shaders/MergedPipelineReflection.h"

namespace Ignis
{
GRIPipelineStatePtr MaterialFactory::build_pso(const CacheKey& key)
{
    const GRIVertexDeclaration* vd = key.layout.empty() ? nullptr : VertexDeclarationRegistry::get().find(key.layout);
    IG_CORE_ASSERT(vd || key.layout.empty(), "MaterialFactory: vertex layout not registered");

    GRIPipelineStateDesc desc;
    desc.vertex_shader        = key.vs;
    desc.pixel_shader         = key.ps;
    desc.vertex_declaration   = const_cast<GRIVertexDeclaration*>(vd);
    desc.render_target_format = key.rt_format;
    desc.depth_stencil_format = key.depth_format;
    desc.primitive_topology   = GRIPrimitiveTopology::TriangleList;
    desc.depth_stencil        = key.depth_stencil;
    desc.raster               = key.raster;
    desc.blend                = key.blend;

    return RenderSystem::get_gri()->create_graphics_pipeline_state(desc);
}

GRIPipelineState* MaterialFactory::get_depth_pso(const CacheKey& forward_key)
{
    GRIDepthStencilDesc ds;
    ds.depth_test  = true;
    ds.depth_write = true;
    ds.depth_func  = GRICompareFunc::LessEqual;

    CacheKey depth_key{forward_key.vs,
                       nullptr,
                       forward_key.layout,
                       GRIPixelFormat::Unknown,
                       GRIPixelFormat::Depth32Float,
                       ds,
                       forward_key.raster,
                       GRIBlendDesc{}};
    auto     it = m_pso_cache.find(depth_key);
    if (it == m_pso_cache.end())
    {
        it = m_pso_cache.emplace(depth_key, build_pso(depth_key)).first;
    }
    return it->second.get();
}

SharedPtr<Material> MaterialFactory::get_or_create(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps,
                                                   const String& layout, GRIPixelFormat rt_fmt,
                                                   GRIPixelFormat depth_fmt, const GRIDepthStencilDesc& depth_stencil,
                                                   const GRIRasterDesc& raster, const GRIBlendDesc& blend)
{
    CacheKey key{
        vs->get_shader(), ps ? ps->get_shader() : nullptr, layout, rt_fmt, depth_fmt, depth_stencil, raster, blend};

    auto mat_it = m_mat_cache.find(key);
    if (mat_it != m_mat_cache.end())
    {
        return mat_it->second;
    }

    auto pso_it = m_pso_cache.find(key);
    if (pso_it == m_pso_cache.end())
    {
        pso_it = m_pso_cache.emplace(key, build_pso(key)).first;
    }

    const GRIVertexDeclaration* vd = key.layout.empty() ? nullptr : VertexDeclarationRegistry::get().find(key.layout);
    GRIPipelineState*           depth_pso = get_depth_pso(key);

    MergedPipelineReflection merged =
        MergedPipelineReflection::merge(vs ? &vs->get_reflection() : nullptr, ps ? &ps->get_reflection() : nullptr);

    auto mat =
        create_shared<Material>(vs, ps, pso_it->second, vd, std::move(merged), nullptr, depth_pso, raster, blend);
    m_mat_cache.emplace(key, mat);
    return mat;
}

SharedPtr<Material> MaterialFactory::create_with_params(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps,
                                                        const String& layout, GRIPixelFormat rt_fmt,
                                                        GRIPixelFormat             depth_fmt,
                                                        const GRIDepthStencilDesc& depth_stencil,
                                                        const GRIRasterDesc& raster, const GRIBlendDesc& blend,
                                                        GRIBufferPtr params, PBRMaterialParams pbr_params)
{
    auto& gc = Renderer::get_global_cache();
    if (!pbr_params.albedo_tex)
    {
        pbr_params.albedo_tex = gc.get_default_white_idx();
    }
    if (!pbr_params.normal_tex)
    {
        pbr_params.normal_tex = gc.get_default_normal_idx();
    }
    if (!pbr_params.roughness_tex)
    {
        pbr_params.roughness_tex = gc.get_default_gray_idx();
    }
    if (!pbr_params.metallic_tex)
    {
        pbr_params.metallic_tex = gc.get_default_black_idx();
    }
    if (!pbr_params.ao_tex)
    {
        pbr_params.ao_tex = gc.get_default_white_idx();
    }
    if (!pbr_params.emissive_tex)
    {
        pbr_params.emissive_tex = gc.get_default_black_idx();
    }
    CacheKey key{
        vs->get_shader(), ps ? ps->get_shader() : nullptr, layout, rt_fmt, depth_fmt, depth_stencil, raster, blend};

    auto pso_it = m_pso_cache.find(key);
    if (pso_it == m_pso_cache.end())
    {
        pso_it = m_pso_cache.emplace(key, build_pso(key)).first;
    }

    const GRIVertexDeclaration* vd = key.layout.empty() ? nullptr : VertexDeclarationRegistry::get().find(key.layout);
    GRIPipelineState*           depth_pso = get_depth_pso(key);

    MergedPipelineReflection merged =
        MergedPipelineReflection::merge(vs ? &vs->get_reflection() : nullptr, ps ? &ps->get_reflection() : nullptr);

    return create_shared<Material>(vs, ps, pso_it->second, vd, std::move(merged), std::move(params), depth_pso, raster,
                                   blend, pbr_params);
}

void MaterialFactory::clear()
{
    m_mat_cache.clear();
    m_pso_cache.clear();
}

void MaterialFactory::evict(RenderShader* vs, RenderShader* ps)
{
    GRIShader* vs_gri = vs ? vs->get_shader() : nullptr;
    GRIShader* ps_gri = ps ? ps->get_shader() : nullptr;

    for (auto it = m_pso_cache.begin(); it != m_pso_cache.end();)
    {
        if (it->first.vs == vs_gri || it->first.ps == ps_gri)
        {
            it = m_pso_cache.erase(it);
        }
        else
        {
            ++it;
        }
    }
    for (auto it = m_mat_cache.begin(); it != m_mat_cache.end();)
    {
        if (it->first.vs == vs_gri || it->first.ps == ps_gri)
        {
            it = m_mat_cache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
} // namespace Ignis
