#pragma once
#include <Ignis.h>

namespace Ignis
{

struct ProjectDescriptor
{
    String name;
    Path   root;
    String asset_source_dir;   // relative to root
    String compiled_cache_dir; // relative to root
};

} // namespace Ignis
