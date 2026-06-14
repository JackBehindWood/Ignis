#include "edpch.h"
#include "IBLBaker.h"

#include <Ignis/Rendering/RenderSystem.h>
#include <Ignis/Rendering/Shaders/ShaderCache.h>
#include <Ignis/Rendering/GlobalEngineCache.h>
#include <Ignis/Asset/AssetTypes.h>
#include <Ignis/Asset/AssetTexture2D.h>

#include <stb/stb_image.h>

namespace Ignis
{

static constexpr uint32_t k_env_size          = 1024;
static constexpr uint32_t k_irr_size          = 32;
static constexpr uint32_t k_prefilter_size    = 256;
static constexpr uint32_t k_brdf_size         = 512;
static constexpr uint32_t k_prefilter_mips    = 5;
static constexpr uint32_t k_prefilter_samples = 1024;
static constexpr uint32_t k_brdf_samples      = 1024;
static constexpr uint32_t k_uniform_buf_size  = 32 * 1024;

struct alignas(16) EquirectParams
{
    uint32_t face_index;
    uint32_t face_size;
    uint32_t _pad[2];
};

struct alignas(16) IrradianceParams
{
    uint32_t face_index;
    uint32_t face_size;
    uint32_t _pad[2];
};

struct alignas(16) PrefilterParams
{
    uint32_t face_index;
    uint32_t mip_size;
    float    roughness;
    uint32_t num_samples;
    uint32_t env_res;
    uint32_t _pad[3];
};

GRIComputePipelineStatePtr IBLBaker::compile_compute(const Path& hlsl_path, const char* entry)
{
    ShaderCompilerOptions opts;
    opts.stages[0] = {GRIShaderStage::Compute, entry};
    opts.count     = 1;

    SharedPtr<RenderShader> cs = ShaderCache::get().get_or_compile(hlsl_path, GRIShaderStage::Compute, opts);
    if (!cs)
    {
        IG_CORE_ERROR("IBLBaker: failed to compile {}", hlsl_path.string());
        return nullptr;
    }

    const ShaderReflection&     refl = cs->get_reflection();
    GRIComputePipelineStateDesc desc;
    desc.compute_shader     = cs->get_shader();
    desc.threadgroup_size_x = refl.threadgroup_size_x ? refl.threadgroup_size_x : 8;
    desc.threadgroup_size_y = refl.threadgroup_size_y ? refl.threadgroup_size_y : 8;
    desc.threadgroup_size_z = refl.threadgroup_size_z ? refl.threadgroup_size_z : 1;

    GRIComputePipelineStatePtr pso = RenderSystem::get_gri()->create_compute_pipeline_state(desc);
    IG_CORE_ASSERT(pso, "IBLBaker: PSO creation failed");
    return pso;
}

void IBLBaker::init(const Path& engine_shaders)
{
    const Path ibl_dir = engine_shaders / "ibl";

    m_equirect_pso   = compile_compute(ibl_dir / "equirect_to_cube.hlsl", "CSMain");
    m_irradiance_pso = compile_compute(ibl_dir / "irradiance.hlsl", "CSMain");
    m_prefilter_pso  = compile_compute(ibl_dir / "prefilter.hlsl", "CSPrefilter");
    m_brdf_pso       = compile_compute(ibl_dir / "brdf_lut.hlsl", "CSMain");

    GRISamplerDesc sd;
    sd.linear_filter = true;
    sd.clamp_to_edge = true;
    m_sampler        = RenderSystem::get_gri()->create_sampler_state(sd);

    GRIBufferDesc null_desc;
    null_desc.size    = 16;
    null_desc.usage   = GRIBufferUsage::UniformBuffer | GRIBufferUsage::Dynamic;
    m_null_space0_buf = RenderSystem::get_gri()->create_buffer(null_desc);

    m_uniform_alloc.init(k_uniform_buf_size);
}

void IBLBaker::dispatch_face(GRICommandList& cmd, const FaceParams& p)
{
    FrameUniformAllocator::Allocation ubo = m_uniform_alloc.allocate(p.params_data, p.params_size);

    Vector<GRITexture2D*> storage_textures;
    if (p.input_tex)
    {
        storage_textures.push_back(p.input_tex);
    }
    storage_textures.push_back(p.output);

    cmd.begin_compute_pass({m_null_space0_buf.get()}, std::move(storage_textures));
    cmd.set_compute_pipeline_state(p.pso);
    cmd.set_uniform_buffer(ubo.buffer, 8, GRIShaderStage::Compute, ubo.offset);
    cmd.set_storage_buffer(m_null_space0_buf.get(), 0);
    if (p.input_tex)
    {
        cmd.set_storage_texture(p.input_tex, 0, 0, 0);
    }
    cmd.set_storage_texture(p.output, 1, p.mip, p.face);
    cmd.set_compute_sampler(m_sampler.get(), 0);

    const uint32_t groups_x = (p.mip_size + 7) / 8;
    const uint32_t groups_y = (p.mip_size + 7) / 8;
    cmd.dispatch(groups_x, groups_y, 1);
    cmd.end_compute_pass();
}

IBLBakeResult IBLBaker::bake_environment(const Path& equirect_hdr_path)
{
    int    w = 0, h = 0, channels = 0;
    float* hdr_data = stbi_loadf(equirect_hdr_path.string().c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if (!hdr_data)
    {
        IG_CORE_ERROR("IBLBaker: failed to load HDR: {}", equirect_hdr_path.string());
        return {};
    }

    GRITexture2DDesc equirect_desc;
    equirect_desc.width             = static_cast<uint32_t>(w);
    equirect_desc.height            = static_cast<uint32_t>(h);
    equirect_desc.format            = GRIPixelFormat::RGBA16Float;
    equirect_desc.initial_data      = hdr_data;
    equirect_desc.initial_data_size = static_cast<size_t>(w) * h * 4 * sizeof(float);
    GRITexture2DPtr equirect_tex    = RenderSystem::get_gri()->create_texture2d(equirect_desc);
    stbi_image_free(hdr_data);

    GRITexture2DDesc cube_desc;
    cube_desc.format                 = GRIPixelFormat::RGBA16Float;
    cube_desc.allow_unordered_access = true;

    cube_desc.width = cube_desc.height = k_env_size;
    cube_desc.num_mip_levels           = 1;
    GRITexture2DPtr env_cube           = RenderSystem::get_gri()->create_cubemap(cube_desc);

    cube_desc.width = cube_desc.height = k_irr_size;
    cube_desc.num_mip_levels           = 1;
    GRITexture2DPtr irr_cube           = RenderSystem::get_gri()->create_cubemap(cube_desc);

    cube_desc.width = cube_desc.height = k_prefilter_size;
    cube_desc.num_mip_levels           = k_prefilter_mips;
    GRITexture2DPtr prefilter_cube     = RenderSystem::get_gri()->create_cubemap(cube_desc);

    GRICommandList& cmd = RenderSystem::get_command_list();
    cmd.begin_frame();
    m_uniform_alloc.begin_frame();

    for (uint32_t face = 0; face < 6; ++face)
    {
        EquirectParams params{face, k_env_size, {0, 0}};
        FaceParams     fp;
        fp.pso         = m_equirect_pso.get();
        fp.input_tex   = equirect_tex.get();
        fp.output      = env_cube.get();
        fp.face        = face;
        fp.mip         = 0;
        fp.mip_size    = k_env_size;
        fp.params_data = &params;
        fp.params_size = sizeof(params);
        dispatch_face(cmd, fp);
    }

    for (uint32_t face = 0; face < 6; ++face)
    {
        IrradianceParams params{face, k_irr_size, {0, 0}};
        FaceParams       fp;
        fp.pso         = m_irradiance_pso.get();
        fp.input_tex   = env_cube.get();
        fp.output      = irr_cube.get();
        fp.face        = face;
        fp.mip         = 0;
        fp.mip_size    = k_irr_size;
        fp.params_data = &params;
        fp.params_size = sizeof(params);
        dispatch_face(cmd, fp);
    }

    for (uint32_t mip = 0; mip < k_prefilter_mips; ++mip)
    {
        const uint32_t mip_size = k_prefilter_size >> mip;
        const float    roughness =
            (k_prefilter_mips > 1) ? (static_cast<float>(mip) / static_cast<float>(k_prefilter_mips - 1)) : 0.0f;

        for (uint32_t face = 0; face < 6; ++face)
        {
            PrefilterParams params;
            params.face_index  = face;
            params.mip_size    = mip_size;
            params.roughness   = roughness;
            params.num_samples = k_prefilter_samples;
            params.env_res     = k_env_size;
            params._pad[0] = params._pad[1] = params._pad[2] = 0;

            FaceParams fp;
            fp.pso         = m_prefilter_pso.get();
            fp.input_tex   = env_cube.get();
            fp.output      = prefilter_cube.get();
            fp.face        = face;
            fp.mip         = mip;
            fp.mip_size    = mip_size;
            fp.params_data = &params;
            fp.params_size = sizeof(params);
            dispatch_face(cmd, fp);
        }
    }

    m_uniform_alloc.end_frame();
    cmd.end_frame();
    RenderSystem::submit();

    IBLBakeResult result;
    result.env_cube        = std::move(env_cube);
    result.irradiance_cube = std::move(irr_cube);
    result.prefilter_cube  = std::move(prefilter_cube);
    return result;
}

GRITexture2DPtr IBLBaker::bake_brdf_lut(const Path& engine_cache_dir)
{
    GRITexture2DDesc lut_desc;
    lut_desc.width                  = k_brdf_size;
    lut_desc.height                 = k_brdf_size;
    lut_desc.format                 = GRIPixelFormat::RG16Float;
    lut_desc.allow_unordered_access = true;
    GRITexture2DPtr lut             = RenderSystem::get_gri()->create_texture2d(lut_desc);

    GRICommandList& cmd = RenderSystem::get_command_list();
    cmd.begin_frame();
    m_uniform_alloc.begin_frame();

    cmd.begin_compute_pass({m_null_space0_buf.get()}, {lut.get()});
    cmd.set_compute_pipeline_state(m_brdf_pso.get());
    cmd.set_storage_buffer(m_null_space0_buf.get(), 0);
    cmd.set_storage_texture(lut.get(), 0, 0, 0);
    cmd.dispatch((k_brdf_size + 7) / 8, (k_brdf_size + 7) / 8, 1);
    cmd.end_compute_pass();

    m_uniform_alloc.end_frame();
    cmd.end_frame();
    RenderSystem::submit();

    Vector<uint8_t> pixels;
    RenderSystem::get_gri()->read_texture_sync(lut.get(), 0, 0, pixels);

    const Path cache_path = engine_cache_dir / "brdf_lut.igasset";
    Filesystem::create_directories(engine_cache_dir);

    std::ofstream ofs(cache_path, std::ios::binary);
    if (ofs)
    {
        const char     magic[4] = {'I', 'G', 'A', 'S'};
        const uint8_t  version  = 3;
        const uint8_t  fmt      = static_cast<uint8_t>(AssetPixelFormat::RG16Float);
        const uint32_t width    = k_brdf_size;
        const uint32_t height   = k_brdf_size;
        const uint8_t  is_cube  = 0;
        const uint32_t faces    = 1;
        const uint32_t mips     = 1;

        ofs.write(magic, 4);
        ofs.write(reinterpret_cast<const char*>(&version), 1);
        ofs.write(reinterpret_cast<const char*>(&fmt), 1);
        ofs.write(reinterpret_cast<const char*>(&width), 4);
        ofs.write(reinterpret_cast<const char*>(&height), 4);
        ofs.write(reinterpret_cast<const char*>(&is_cube), 1);
        ofs.write(reinterpret_cast<const char*>(&faces), 4);
        ofs.write(reinterpret_cast<const char*>(&mips), 4);
        ofs.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
    }
    else
    {
        IG_CORE_WARN("IBLBaker: could not write brdf_lut.igasset to {}", engine_cache_dir.string());
    }

    return lut;
}

} // namespace Ignis
