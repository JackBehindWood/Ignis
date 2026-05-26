#pragma once

#include <Ignis.h>
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

    void set_engine_cache_root(const Path& dir);

    void on_project_opened(const ProjectContext& ctx) override;
    void on_project_closed() override;

private:
    EditorShaderCache() = default;
    Path m_engine_cache_root;
};

} // namespace Ignis
