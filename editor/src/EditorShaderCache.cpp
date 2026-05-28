#include "edpch.h"
#include "EditorShaderCache.h"
#include "Ignis/Rendering/Shaders/ShaderCache.h"

namespace Ignis
{

void EditorShaderCache::set_engine_root(const Path& engine_root)
{
    m_shaders_root      = engine_root / "shaders";
    m_engine_cache_root = engine_root / "cache" / "shaders";
    ShaderCache::get().set_engine_cache_root(m_engine_cache_root);
    ShaderCache::get().set_cache_root(m_engine_cache_root);
}

SharedPtr<RenderShader> EditorShaderCache::get_or_compile(const String& filename, GRIShaderStage stage)
{
    return ShaderCache::get().get_or_compile(m_shaders_root / filename, stage);
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
