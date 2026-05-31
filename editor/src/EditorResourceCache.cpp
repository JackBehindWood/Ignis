#include "edpch.h"
#include "EditorResourceCache.h"
#include "EditorPrimitives.h"
#include "Ignis/Rendering/Renderer.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/MaterialFactory.h"

namespace Ignis
{

void EditorResourceCache::init(const Path& engine_root)
{
    UniqueLock lock(m_mutex);
    m_shader_cache.init(engine_root / "shaders", engine_root / "cache" / "shaders");
    compile_all();
}

void EditorResourceCache::shutdown()
{
    UniqueLock lock(m_mutex);
    m_outline_params.reset();
    for (auto& mat : m_materials)
    {
        mat.reset();
    }
}

void EditorResourceCache::reload()
{
    UniqueLock lock(m_mutex);
    m_outline_params.reset();
    for (auto& mat : m_materials)
    {
        mat.reset();
    }
    compile_all();
}

SharedPtr<Material> EditorResourceCache::get_material(EditorMaterial mat) const
{
    SharedLock lock(m_mutex);
    return m_materials[static_cast<size_t>(mat)];
}

GRIBuffer* EditorResourceCache::get_outline_params() const
{
    SharedLock lock(m_mutex);
    return m_outline_params.get();
}

void EditorResourceCache::on_project_opened(const ProjectContext& ctx)
{
    m_shader_cache.on_project_opened(ctx);
}

void EditorResourceCache::on_project_closed()
{
    m_shader_cache.on_project_closed();
}

void EditorResourceCache::compile_all()
{
    const RendererConfig& cfg = Renderer::get_config();

    SharedPtr<RenderShader> grid_vs = m_shader_cache.get_or_compile("grid.hlsl", GRIShaderStage::Vertex);
    SharedPtr<RenderShader> grid_ps = m_shader_cache.get_or_compile("grid.hlsl", GRIShaderStage::Pixel);

    GRIBlendDesc grid_blend;
    grid_blend.enable     = true;
    grid_blend.src_factor = GRIBlendFactor::SrcAlpha;
    grid_blend.dst_factor = GRIBlendFactor::InvSrcAlpha;
    grid_blend.blend_op   = GRIBlendOp::Add;
    grid_blend.src_alpha  = GRIBlendFactor::One;
    grid_blend.dst_alpha  = GRIBlendFactor::InvSrcAlpha;
    grid_blend.alpha_op   = GRIBlendOp::Add;
    GRIRasterDesc grid_raster;
    grid_raster.cull_mode                                  = GRICullMode::None;
    m_materials[static_cast<size_t>(EditorMaterial::Grid)] = Renderer::get_material_factory().get_or_create(
        grid_vs, grid_ps, "", cfg.render_target_format, cfg.depth_format, {}, grid_raster, grid_blend);

    SharedPtr<RenderShader> outline_vs =
        m_shader_cache.get_or_compile("outline_composite.hlsl", GRIShaderStage::Vertex);
    SharedPtr<RenderShader> outline_ps = m_shader_cache.get_or_compile("outline_composite.hlsl", GRIShaderStage::Pixel);

    GRIDepthStencilDesc outline_ds;
    outline_ds.depth_test  = false;
    outline_ds.depth_write = false;
    GRIBlendDesc outline_blend;
    outline_blend.enable     = true;
    outline_blend.src_factor = GRIBlendFactor::SrcAlpha;
    outline_blend.dst_factor = GRIBlendFactor::InvSrcAlpha;
    outline_blend.blend_op   = GRIBlendOp::Add;
    outline_blend.src_alpha  = GRIBlendFactor::One;
    outline_blend.dst_alpha  = GRIBlendFactor::InvSrcAlpha;
    outline_blend.alpha_op   = GRIBlendOp::Add;
    GRIRasterDesc outline_raster;
    outline_raster.cull_mode                                           = GRICullMode::None;
    m_materials[static_cast<size_t>(EditorMaterial::SelectionOutline)] = Renderer::get_material_factory().get_or_create(
        outline_vs, outline_ps, "", cfg.render_target_format, GRIPixelFormat::Unknown, outline_ds, outline_raster,
        outline_blend);

    if (!m_outline_params)
    {
        struct OutlineParams
        {
            float color[4];
        };
        const OutlineParams outline_params = {0.26f, 0.59f, 1.0f, 1.0f};
        GRIBufferDesc       desc;
        desc.size        = sizeof(OutlineParams);
        desc.usage       = GRIBufferUsage::UniformBuffer;
        m_outline_params = RenderSystem::get_gri()->create_buffer(desc, &outline_params);
    }

    EditorPrimitives::init(m_shader_cache);
}

} // namespace Ignis
