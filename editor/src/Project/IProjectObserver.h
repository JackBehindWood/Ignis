#pragma once

#include <Ignis.h>
#include "ProjectDescriptor.h"

namespace Ignis
{

struct ProjectContext
{
    const ProjectDescriptor& descriptor;
    Path                     asset_source_abs;
    Path                     compiled_cache_abs;
    bool                     in_memory = false;
};

class IProjectObserver
{
public:
    virtual void on_project_opened(const ProjectContext& ctx) = 0;
    virtual void on_project_closed()                          = 0;

protected:
    ~IProjectObserver() = default;
};

} // namespace Ignis
