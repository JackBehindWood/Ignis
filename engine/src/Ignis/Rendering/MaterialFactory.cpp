#include "igpch.h"
#include "MaterialFactory.h"

#include "Ignis/Rendering/VertexDeclarationRegistry.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/GRI/GRI.h"

namespace Ignis
{
    GRIPipelineStatePtr MaterialFactory::build_pso(GRIShader* vs, GRIShader* ps, const String& layout)
    {
        const GRIVertexDeclaration* vd = VertexDeclarationRegistry::get().find(layout);
        IG_CORE_ASSERT(vd || layout.empty(), "MaterialFactory: vertex layout not registered");

        GRIPipelineStateDesc desc;
        desc.vertex_shader        = vs;
        desc.pixel_shader         = ps;
        desc.vertex_declaration   = const_cast<GRIVertexDeclaration*>(vd); // GRI only reads during PSO creation
        desc.render_target_format = m_rt_format;
        desc.depth_stencil_format = m_depth_format;
        desc.primitive_topology   = GRIPrimitiveTopology::TriangleList;

        return RenderSystem::get_gri()->create_graphics_pipeline_state(desc);
    }

    SharedPtr<Material> MaterialFactory::get_or_create(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps,
                                                        const String& layout)
    {
        CacheKey key{ vs->get_shader(), ps->get_shader(), layout };

        auto mat_it = m_mat_cache.find(key);
        if (mat_it != m_mat_cache.end())
            return mat_it->second;

        auto pso_it = m_pso_cache.find(key);
        if (pso_it == m_pso_cache.end())
            pso_it = m_pso_cache.emplace(key, build_pso(key.vs, key.ps, layout)).first;

        const GRIVertexDeclaration* vd = VertexDeclarationRegistry::get().find(layout);

        auto mat = create_shared<Material>(vs, ps, pso_it->second, vd, nullptr);
        m_mat_cache.emplace(key, mat);
        return mat;
    }

    SharedPtr<Material> MaterialFactory::create_with_params(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps,
                                                             const String& layout, GRIBufferPtr params)
    {
        CacheKey key{ vs->get_shader(), ps->get_shader(), layout };

        auto pso_it = m_pso_cache.find(key);
        if (pso_it == m_pso_cache.end())
            pso_it = m_pso_cache.emplace(key, build_pso(key.vs, key.ps, layout)).first;

        const GRIVertexDeclaration* vd = VertexDeclarationRegistry::get().find(layout);

        return create_shared<Material>(vs, ps, pso_it->second, vd, std::move(params));
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

        for (auto it = m_pso_cache.begin(); it != m_pso_cache.end(); )
        {
            if (it->first.vs == vs_gri || it->first.ps == ps_gri)
                it = m_pso_cache.erase(it);
            else
                ++it;
        }
        for (auto it = m_mat_cache.begin(); it != m_mat_cache.end(); )
        {
            if (it->first.vs == vs_gri || it->first.ps == ps_gri)
                it = m_mat_cache.erase(it);
            else
                ++it;
        }
    }
} // namespace Ignis
