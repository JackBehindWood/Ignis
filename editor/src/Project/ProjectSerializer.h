#pragma once
#include "ProjectDescriptor.h"

namespace Ignis
{

struct ProjectSerializer
{
    static bool read(const Path& path, ProjectDescriptor& out);
    static bool write(const Path& path, const ProjectDescriptor& desc);
};

} // namespace Ignis
