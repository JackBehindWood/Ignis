#pragma once
#include <Ignis.h>

namespace Ignis
{

struct EditorSettings
{
    static constexpr uint32_t k_max_recent_projects = 10;
    Vector<Path>              recent_projects;
    Path                      projects_root;
};

} // namespace Ignis
