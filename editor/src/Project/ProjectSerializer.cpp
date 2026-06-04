#include "edpch.h"
#include "ProjectSerializer.h"

#include "Ignis/Foundation/YamlStream.h"

namespace Ignis
{

static constexpr int k_current_project_version = 1;

bool ProjectSerializer::read(const Path& path, ProjectDescriptor& out)
{
    YamlReader r(path);
    if (!r.is_open())
    {
        IG_ERROR("ProjectSerializer: cannot open '{0}'", path.string());
        return false;
    }

    const int version = r.get<int>("project_version", -1);
    if (version < 0)
    {
        IG_ERROR("ProjectSerializer: missing 'project_version' in '{0}'", path.string());
        return false;
    }
    if (version > k_current_project_version)
    {
        IG_ERROR("ProjectSerializer: unsupported project_version {0} in '{1}'", version, path.string());
        return false;
    }

    const String name               = r.get<String>("name", {});
    const String asset_source_dir   = r.get<String>("asset_source_dir", {});
    const String compiled_cache_dir = r.get<String>("compiled_cache_dir", {});

    if (name.empty() || asset_source_dir.empty() || compiled_cache_dir.empty())
    {
        IG_ERROR("ProjectSerializer: missing required field(s) in '{0}'", path.string());
        return false;
    }

    out.name               = name;
    out.root               = path.parent_path();
    out.asset_source_dir   = asset_source_dir;
    out.compiled_cache_dir = compiled_cache_dir;
    out.startup_scene      = r.get<String>("startup_scene", {});
    return true;
}

bool ProjectSerializer::write(const Path& path, const ProjectDescriptor& desc)
{
    YamlWriter w;
    w.set("project_version", k_current_project_version)
        .set("name", desc.name)
        .set("asset_source_dir", desc.asset_source_dir)
        .set("compiled_cache_dir", desc.compiled_cache_dir);
    if (!desc.startup_scene.empty())
    {
        w.set("startup_scene", desc.startup_scene);
    }

    if (!w.write(path))
    {
        IG_ERROR("ProjectSerializer: cannot write '{0}'", path.string());
        return false;
    }
    return true;
}

} // namespace Ignis
