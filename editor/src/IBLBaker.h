#pragma once

#include <Ignis.h>
#include <Ignis/Rendering/GRI/GRIDefinitions.h>
#include <Ignis/Rendering/GRI/GRICommandList.h>
#include <Ignis/Rendering/FrameUniformAllocator.h>

namespace Ignis
{

struct IBLBakeResult
{
    GRITexture2DPtr env_cube;        // 1024×1024×6 RGBA16F
    GRITexture2DPtr irradiance_cube; // 32×32×6 RGBA16F
    GRITexture2DPtr prefilter_cube;  // 256×256×6 RGBA16F, 5 mips
    GRITexture2DPtr brdf_lut;        // 512×512 RG16F 2D
};

class IBLBaker
{
public:
    void init(const Path& engine_shaders);

    IBLBakeResult   bake_environment(const Path& equirect_hdr_path);
    GRITexture2DPtr bake_brdf_lut(const Path& engine_cache_dir);

private:
    GRIComputePipelineStatePtr compile_compute(const Path& hlsl_path, const char* entry);

    struct FaceParams
    {
        GRIComputePipelineState* pso;
        GRIBuffer*               input_buf; // null for brdf_lut (no UBO slot 8)
        GRITexture2D*            input_tex; // t0 — full texture or null
        GRITexture2D*            output;    // u1 — cubemap being written
        uint32_t                 face;
        uint32_t                 mip;
        uint32_t                 mip_size;
        uint32_t                 params_size; // bytes to upload to UBO
        const void*              params_data;
    };

    void dispatch_face(GRICommandList& cmd, const FaceParams& p);

    GRIComputePipelineStatePtr m_equirect_pso;
    GRIComputePipelineStatePtr m_irradiance_pso;
    GRIComputePipelineStatePtr m_prefilter_pso;
    GRIComputePipelineStatePtr m_brdf_pso;
    GRISamplerStatePtr         m_sampler;
    GRIBufferPtr               m_null_space0_buf;
    FrameUniformAllocator      m_uniform_alloc;
};

} // namespace Ignis
