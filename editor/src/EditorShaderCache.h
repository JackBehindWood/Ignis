#pragma once

#include <Ignis.h>
#include <Ignis/Rendering/GRI/GRIDefinitions.h>
#include <Ignis/Rendering/Shaders/RenderShader.h>
#include "Project/IProjectObserver.h"

namespace Ignis
{

class EditorShaderCache : public IProjectObserver
{
public:
    static EditorShaderCache& get()
    {
        static EditorShaderCache instance;
        return instance;
    }

    void set_engine_root(const Path& engine_root);

    SharedPtr<RenderShader> get_or_compile(const String& filename, GRIShaderStage stage);

    void on_project_opened(const ProjectContext& ctx) override;
    void on_project_closed() override;

private:
    EditorShaderCache() = default;
    Path m_shaders_root;
    Path m_engine_cache_root;
};

} // namespace Ignis
