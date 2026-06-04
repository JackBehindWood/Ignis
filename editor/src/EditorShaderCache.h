#pragma once

#include <Ignis.h>
#include <Ignis/Rendering/GRI/GRIDefinitions.h>
#include <Ignis/Rendering/Shaders/RenderShader.h>
#include "Project/IProjectObserver.h"

namespace Ignis
{

class EditorShaderCache
{
public:
    void init(const Path& shaders_root, const Path& engine_cache_root);
    void on_project_opened(const ProjectContext& ctx);
    void on_project_closed();

    SharedPtr<RenderShader> get_or_compile(const String& filename, GRIShaderStage stage);
    SharedPtr<RenderShader> get_or_compile_path(const Path& full_path, GRIShaderStage stage);
    SharedPtr<RenderShader> get_or_compile_from_source(const String& virtual_name, const String& hlsl_source,
                                                       GRIShaderStage stage);

private:
    Path m_shaders_root;
    Path m_engine_cache_root;
};

} // namespace Ignis
