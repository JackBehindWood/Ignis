#include "edpch.h"
#include "EditorShaderCache.h"
#include "Ignis/Rendering/Shaders/ShaderCache.h"

namespace Ignis
{

void EditorShaderCache::set_engine_cache_root(const Path& dir)
{
    m_engine_cache_root = dir;
    ShaderCache::get().set_engine_cache_root(dir);
    ShaderCache::get().set_cache_root(dir);
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
