#include "edpch.h"
#include "EditorShaderCache.h"
#include "Ignis/Rendering/Shaders/ShaderCache.h"

namespace Ignis
{

void EditorShaderCache::init(const Path& shaders_root, const Path& engine_cache_root)
{
    m_shaders_root      = shaders_root;
    m_engine_cache_root = engine_cache_root;
    ShaderCache::get().set_engine_cache_root(m_engine_cache_root);
    ShaderCache::get().set_cache_root(m_engine_cache_root);
}

SharedPtr<RenderShader> EditorShaderCache::get_or_compile(const String& filename, GRIShaderStage stage)
{
    return ShaderCache::get().get_or_compile(m_shaders_root / filename, stage);
}

SharedPtr<RenderShader> EditorShaderCache::get_or_compile_path(const Path& full_path, GRIShaderStage stage)
{
    return ShaderCache::get().get_or_compile(full_path, stage);
}

SharedPtr<RenderShader> EditorShaderCache::get_or_compile_from_source(const String& virtual_name,
                                                                      const String& hlsl_source, GRIShaderStage stage)
{
    ShaderCompilerOptions opts;
    opts.count           = 2;
    opts.stages[0].stage = GRIShaderStage::Vertex;
    opts.stages[1].stage = GRIShaderStage::Pixel;
    return ShaderCache::get().get_or_compile(hlsl_source, virtual_name, stage, opts);
}

void EditorShaderCache::on_project_opened(const ProjectContext& ctx)
{
    const Path project_shader_cache = ctx.in_memory ? m_engine_cache_root : (ctx.compiled_cache_abs / "shaders");
    ShaderCache::get().set_cache_root(project_shader_cache);
}

void EditorShaderCache::on_project_closed()
{
    ShaderCache::get().set_cache_root(m_engine_cache_root);
}

} // namespace Ignis
