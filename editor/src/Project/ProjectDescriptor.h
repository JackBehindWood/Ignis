#pragma once
#include <Ignis.h>

namespace Ignis
{

// TODO: use path for asset_source_dir, compiled_cache_dir, and startup_scene instead of strings.
struct ProjectDescriptor
{
    String name;
    Path   root;
    String asset_source_dir;   // relative to root
    String compiled_cache_dir; // relative to root
    String startup_scene;      // relative to root; empty = no startup scene
};

} // namespace Ignis
