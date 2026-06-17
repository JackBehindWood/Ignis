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
    SharedPtr<RenderShader> get_pbr_ps_masked() const
    {
        SharedLock lock(m_mutex);
        return m_pbr_ps_masked;
    }
    void set_pbr_shaders(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps,
                         SharedPtr<RenderShader> ps_masked = nullptr)
    {
        UniqueLock lock(m_mutex);
        m_pbr_vs        = std::move(vs);
        m_pbr_ps        = std::move(ps);
        m_pbr_ps_masked = std::move(ps_masked);
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

    void set_default_texture_indices(uint32_t white, uint32_t normal, uint32_t black, uint32_t gray)
    {
        UniqueLock lock(m_mutex);
        m_default_white_idx  = white;
        m_default_normal_idx = normal;
        m_default_black_idx  = black;
        m_default_gray_idx   = gray;
    }

    uint32_t get_default_white_idx() const
    {
        SharedLock lock(m_mutex);
        return m_default_white_idx;
    }
    uint32_t get_default_normal_idx() const
    {
        SharedLock lock(m_mutex);
        return m_default_normal_idx;
    }
    uint32_t get_default_black_idx() const
    {
        SharedLock lock(m_mutex);
        return m_default_black_idx;
    }
    uint32_t get_default_gray_idx() const
    {
        SharedLock lock(m_mutex);
        return m_default_gray_idx;
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
        m_pbr_ps_masked.reset();
        m_cull_pso.reset();
        m_default_white_idx  = 0;
        m_default_normal_idx = 0;
        m_default_black_idx  = 0;
        m_default_gray_idx   = 0;
    }

private:
    mutable SharedMutex        m_mutex;
    SharedPtr<RenderTexture2D> m_brdf_lut;
    SharedPtr<RenderTexture2D> m_irradiance_cube;
    SharedPtr<RenderTexture2D> m_prefilter_cube;
    SharedPtr<Material>        m_tonemap_material;
    SharedPtr<RenderShader>    m_pbr_vs;
    SharedPtr<RenderShader>    m_pbr_ps;
    SharedPtr<RenderShader>    m_pbr_ps_masked;
    GRIComputePipelineStatePtr m_cull_pso;
    uint32_t                   m_default_white_idx  = 0;
    uint32_t                   m_default_normal_idx = 0;
    uint32_t                   m_default_black_idx  = 0;
    uint32_t                   m_default_gray_idx   = 0;
};

} // namespace Ignis
