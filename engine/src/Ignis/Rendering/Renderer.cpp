#include "igpch.h"
#include "Renderer.h"

#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/VertexDeclarationRegistry.h"
#include "Ignis/Rendering/Shaders/ShaderCache.h"

namespace Ignis
{

static constexpr uint32_t k_default_vb_slot = static_cast<uint32_t>(DefaultBindings::VertexBuffer);

static void register_builtin_vertex_layouts()
{
    if (VertexDeclarationRegistry::get().find("standard_mesh"))
    {
        return;
    }
    auto vd          = create_shared<GRIVertexDeclaration>();
    vd->num_elements = 3;
    vd->elements[0]  = {GRIVertexElementSemantic::Position, GRIVertexElementFormat::Float3, 0, k_default_vb_slot};
    vd->elements[1]  = {GRIVertexElementSemantic::Normal, GRIVertexElementFormat::Float3, 12, k_default_vb_slot};
    vd->elements[2]  = {GRIVertexElementSemantic::TexCoord, GRIVertexElementFormat::Float2, 24, k_default_vb_slot};
    vd->num_bindings = 1;
    vd->bindings[0]  = {k_default_vb_slot, 32};
    VertexDeclarationRegistry::get().register_layout("standard_mesh", std::move(vd));
}

void Renderer::init(const RendererConfig& config)
{
    s_config = config;
    s_material_factory.set_render_target_format(config.render_target_format);
    s_material_factory.set_depth_format(config.depth_format);
    s_frame_alloc.init(config.uniform_buffer_size);
    register_builtin_vertex_layouts();
    if (!config.engine_shaders.empty())
    {
        ShaderCache::get().set_engine_include_dir(config.engine_shaders / "include");
        warmup_system_shaders(config.engine_shaders);
    }
}

void Renderer::warmup_system_shaders(const Path& engine_shaders_root)
{
    SharedPtr<RenderShader> tm_vs =
        ShaderCache::get().get_or_compile(engine_shaders_root / "tonemap.hlsl", GRIShaderStage::Vertex);
    SharedPtr<RenderShader> tm_ps =
        ShaderCache::get().get_or_compile(engine_shaders_root / "tonemap.hlsl", GRIShaderStage::Pixel);
    if (tm_vs && tm_ps)
    {
        GRIDepthStencilDesc ds;
        ds.depth_test           = false;
        ds.depth_write          = false;
        SharedPtr<Material> mat = s_material_factory.get_or_create(tm_vs, tm_ps, "", GRIPixelFormat::BGRA8Unorm,
                                                                   GRIPixelFormat::Unknown, ds, {}, {});
        s_global_cache.set_tonemap_material(std::move(mat));
    }

    ShaderCompilerOptions pbr_opaque_opts;
    pbr_opaque_opts.defines = {{"IG_MASKED", "0"}};
    ShaderCompilerOptions pbr_masked_opts;
    pbr_masked_opts.defines = {{"IG_MASKED", "1"}};

    SharedPtr<RenderShader> pbr_vs =
        ShaderCache::get().get_or_compile(engine_shaders_root / "pbr.hlsl", GRIShaderStage::Vertex, pbr_opaque_opts);
    SharedPtr<RenderShader> pbr_ps =
        ShaderCache::get().get_or_compile(engine_shaders_root / "pbr.hlsl", GRIShaderStage::Pixel, pbr_opaque_opts);
    SharedPtr<RenderShader> pbr_ps_masked =
        ShaderCache::get().get_or_compile(engine_shaders_root / "pbr.hlsl", GRIShaderStage::Pixel, pbr_masked_opts);
    if (pbr_vs && pbr_ps)
    {
        GRIShader* pbr_ps_raw = pbr_ps->get_shader();
        s_global_cache.set_pbr_shaders(std::move(pbr_vs), std::move(pbr_ps), std::move(pbr_ps_masked));
        RenderSystem::get_gri()->init_bindless_array(pbr_ps_raw);

        GRISamplerStatePtr default_sampler = RenderSystem::get_gri()->create_sampler_state({});

        struct TexDef
        {
            const uint8_t* data;
            const char*    name;
        };
        static constexpr uint8_t k_white[4]  = {0xFF, 0xFF, 0xFF, 0xFF};
        static constexpr uint8_t k_normal[4] = {0x80, 0x80, 0xFF, 0xFF};
        static constexpr uint8_t k_black[4]  = {0x00, 0x00, 0x00, 0xFF};
        static constexpr uint8_t k_gray[4]   = {0x80, 0x80, 0x80, 0xFF};
        const uint8_t*           defs[4]     = {k_white, k_normal, k_black, k_gray};
        uint32_t                 indices[4]  = {};
        for (int i = 0; i < 4; ++i)
        {
            GRITexture2DDesc td;
            td.width             = 1;
            td.height            = 1;
            td.format            = GRIPixelFormat::RGBA8Unorm;
            td.initial_data      = defs[i];
            td.initial_data_size = 4;
            GRITexture2DPtr gri  = RenderSystem::get_gri()->create_texture2d(td);
            IG_CORE_ASSERT(gri, "Renderer: failed to create default texture");
            indices[i] = RenderSystem::get_gri()->register_bindless_texture(std::move(gri), default_sampler);
        }
        s_global_cache.set_default_texture_indices(indices[0], indices[1], indices[2], indices[3]);
    }

    ShaderCompilerOptions cull_opts;
    cull_opts.stages[0] = {GRIShaderStage::Compute};
    cull_opts.count     = 1;

    SharedPtr<RenderShader> cs =
        ShaderCache::get().get_or_compile(engine_shaders_root / "cull.hlsl", GRIShaderStage::Compute, cull_opts);
    if (cs)
    {
        const ShaderReflection&     refl = cs->get_reflection();
        GRIComputePipelineStateDesc desc;
        desc.compute_shader                 = cs->get_shader();
        desc.threadgroup_size_x             = refl.threadgroup_size_x ? refl.threadgroup_size_x : 64;
        desc.threadgroup_size_y             = refl.threadgroup_size_y ? refl.threadgroup_size_y : 1;
        desc.threadgroup_size_z             = refl.threadgroup_size_z ? refl.threadgroup_size_z : 1;
        GRIComputePipelineStatePtr cull_pso = RenderSystem::get_gri()->create_compute_pipeline_state(desc);
        IG_CORE_ASSERT(cull_pso, "Renderer: failed to create GPU cull PSO");
        s_global_cache.set_cull_pipeline_state(std::move(cull_pso));
    }
}

void Renderer::shutdown()
{
    s_frame_alloc.shutdown();
    s_material_factory.clear();
    s_resource_cache.clear();
}

void Renderer::begin_frame(GRIViewport* viewport)
{
    s_frame_alloc.begin_frame();
    s_depth_texture = viewport ? RenderSystem::get_gri()->get_viewport_depth_texture(viewport) : nullptr;

    GRICommandList& cmd = RenderSystem::get_command_list();
    cmd.begin_frame();
    // cmd.begin_drawing_viewport(viewport, nullptr);
}

void Renderer::end_frame()
{
    GRICommandList& cmd = RenderSystem::get_command_list();
    cmd.end_frame();
    RenderSystem::submit();

    s_frame_alloc.end_frame();
}

void Renderer::upload_frame_data(const GPUFrameData& data)
{
    s_frame_data_alloc = s_frame_alloc.allocate(&data, sizeof(GPUFrameData));
}

void Renderer::bind_frame_data(GRICommandListBase& cmd_list)
{
    GRICommandList& cmd = GRICommandList::get(cmd_list);
    cmd.set_uniform_buffer(s_frame_data_alloc.buffer, static_cast<uint32_t>(DefaultBindings::FrameData),
                           GRIShaderStage::Vertex, s_frame_data_alloc.offset);
    cmd.set_uniform_buffer(s_frame_data_alloc.buffer, static_cast<uint32_t>(DefaultBindings::FrameData),
                           GRIShaderStage::Pixel, s_frame_data_alloc.offset);
}

void Renderer::evict(uint64_t key)
{
    s_resource_cache.evict(key);
}

void Renderer::clear_pipeline_cache()
{
    s_material_factory.clear();
}
} // namespace Ignis
