#pragma once

#include "Ignis/Rendering/RenderTexture2D.h"
#include "Ignis/Rendering/Shaders/RenderShader.h"
#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"

namespace Ignis
{

class GlobalEngineCache
{
public:
    SharedPtr<RenderTexture2D> get_brdf_lut() const
    {
        SharedLock lock(m_mutex);
        return m_brdf_lut;
    }
    void set_brdf_lut(SharedPtr<RenderTexture2D> lut)
    {
        UniqueLock lock(m_mutex);
        m_brdf_lut = std::move(lut);
    }

    SharedPtr<RenderTexture2D> get_irradiance_cube() const
    {
        SharedLock lock(m_mutex);
        return m_irradiance_cube;
    }
    void set_irradiance_cube(SharedPtr<RenderTexture2D> cube)
    {
        UniqueLock lock(m_mutex);
        m_irradiance_cube = std::move(cube);
    }

    SharedPtr<RenderTexture2D> get_prefilter_cube() const
    {
        SharedLock lock(m_mutex);
        return m_prefilter_cube;
    }
    void set_prefilter_cube(SharedPtr<RenderTexture2D> cube)
    {
        UniqueLock lock(m_mutex);
        m_prefilter_cube = std::move(cube);
    }

    SharedPtr<Material> get_tonemap_material() const
    {
        SharedLock lock(m_mutex);
        return m_tonemap_material;
    }
    void set_tonemap_material(SharedPtr<Material> mat)
    {
        UniqueLock lock(m_mutex);
        m_tonemap_material = std::move(mat);
    }

    SharedPtr<RenderShader> get_pbr_vs() const
    {
        SharedLock lock(m_mutex);
        return m_pbr_vs;
    }
    SharedPtr<RenderShader> get_pbr_ps() const
    {
        SharedLock lock(m_mutex);
        return m_pbr_ps;
    }
    void set_pbr_shaders(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps)
    {
        UniqueLock lock(m_mutex);
        m_pbr_vs = std::move(vs);
        m_pbr_ps = std::move(ps);
    }

    void set_cull_pipeline_state(GRIComputePipelineStatePtr pso)
    {
        UniqueLock lock(m_mutex);
        m_cull_pso = std::move(pso);
    }

    GRIComputePipelineStatePtr get_cull_pipeline_state() const
    {
        SharedLock lock(m_mutex);
        return m_cull_pso;
    }

    void clear()
    {
        UniqueLock lock(m_mutex);
        m_brdf_lut.reset();
        m_irradiance_cube.reset();
        m_prefilter_cube.reset();
        m_tonemap_material.reset();
        m_pbr_vs.reset();
        m_pbr_ps.reset();
        m_cull_pso.reset();
    }

private:
    mutable SharedMutex        m_mutex;
    SharedPtr<RenderTexture2D> m_brdf_lut;
    SharedPtr<RenderTexture2D> m_irradiance_cube;
    SharedPtr<RenderTexture2D> m_prefilter_cube;
    SharedPtr<Material>        m_tonemap_material;
    SharedPtr<RenderShader>    m_pbr_vs;
    SharedPtr<RenderShader>    m_pbr_ps;
    GRIComputePipelineStatePtr m_cull_pso;
};

} // namespace Ignis
