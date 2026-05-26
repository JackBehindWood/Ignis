#include "edpch.h"
#include "SettingsSerializer.h"

#include "Ignis/Foundation/YamlStream.h"

#include <cstdlib>

namespace Ignis
{

static constexpr int k_current_engine_settings_version = 1;
static constexpr int k_current_editor_settings_version = 1;

static Path default_projects_root()
{
    const char* home = ::getenv("HOME");
    if (!home || home[0] == '\0')
    {
        return Path(".ignis") / "projects";
    }
    return Path(home) / ".ignis" / "projects";
}

bool SettingsSerializer::read_editor_settings(const Path& path, EditorSettings& out)
{
    YamlReader r(path);
    if (!r.is_open())
    {
        if (out.projects_root.empty())
        {
            out.projects_root = default_projects_root();
        }
        return true;
    }

    const int version = r.get<int>("settings_version", k_current_editor_settings_version);
    if (version > k_current_editor_settings_version)
    {
        IG_ERROR("SettingsSerializer: unsupported settings_version {0} in '{1}'", version, path.string());
        return false;
    }

    out.recent_projects.clear();
    for (const auto& s : r.get_sequence("recent_projects"))
    {
        out.recent_projects.emplace_back(s);
    }

    const String root_str = r.get<String>("projects_root", "");
    out.projects_root     = root_str.empty() ? default_projects_root() : Path(root_str);

    return true;
}

bool SettingsSerializer::write_editor_settings(const Path& path, const EditorSettings& settings)
{
    std::error_code ec;
    Filesystem::create_directories(path.parent_path(), ec);
    if (ec)
    {
        IG_ERROR("SettingsSerializer: cannot create directory '{0}': {1}", path.parent_path().string(), ec.message());
        return false;
    }

    YamlWriter w;
    w.set("settings_version", k_current_editor_settings_version);
    w.set("projects_root", settings.projects_root.string());

    w.begin_sequence("recent_projects");
    for (const auto& p : settings.recent_projects)
    {
        w.push_sequence_item(p.string());
    }
    w.end_sequence();

    if (!w.write(path))
    {
        IG_ERROR("SettingsSerializer: cannot write editor settings '{0}'", path.string());
        return false;
    }
    return true;
}

bool SettingsSerializer::read_engine_settings(const Path& path, EngineSettings& out)
{
    YamlReader r(path);
    if (!r.is_open())
    {
        return true;
    }

    const int version = r.get<int>("engine_settings_version", k_current_engine_settings_version);
    if (version > k_current_engine_settings_version)
    {
        IG_ERROR("SettingsSerializer: unsupported engine_settings_version {0} in '{1}'", version, path.string());
        return false;
    }

    out.rendering.vsync        = r.get_nested<bool>("rendering", "vsync", out.rendering.vsync);
    out.rendering.msaa_samples = r.get_nested<uint32_t>("rendering", "msaa_samples", out.rendering.msaa_samples);
    out.rendering.target_fps   = r.get_nested<uint32_t>("rendering", "target_fps", out.rendering.target_fps);

    return true;
}

bool SettingsSerializer::write_engine_settings(const Path& path, const EngineSettings& s)
{
    std::error_code ec;
    Filesystem::create_directories(path.parent_path(), ec);
    if (ec)
    {
        IG_ERROR("SettingsSerializer: cannot create directory '{0}': {1}", path.parent_path().string(), ec.message());
        return false;
    }

    YamlWriter w;
    w.set("engine_settings_version", k_current_engine_settings_version)
        .begin_map("rendering")
        .set("vsync", s.rendering.vsync)
        .set("msaa_samples", s.rendering.msaa_samples)
        .set("target_fps", s.rendering.target_fps)
        .end_map();

    if (!w.write(path))
    {
        IG_ERROR("SettingsSerializer: cannot write engine settings '{0}'", path.string());
        return false;
    }
    return true;
}

} // namespace Ignis
