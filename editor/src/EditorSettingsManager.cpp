#include "edpch.h"
#include "EditorSettingsManager.h"
#include "Project/SettingsSerializer.h"
#include "Ignis/Settings/EngineSettingsManager.h"

namespace Ignis
{

Path EditorSettingsManager::editor_prefs_path()
{
    return Filesystem::current_path() / "resources" / ".ignis" / "editor.cfg";
}

Path EditorSettingsManager::engine_prefs_path()
{
    return Filesystem::current_path() / "resources" / ".ignis" / "engine.cfg";
}

void EditorSettingsManager::push_recent(const Path& p)
{
    auto& projects = m_settings.recent_projects;
    projects.erase(std::remove(projects.begin(), projects.end(), p), projects.end());
    projects.insert(projects.begin(), p);
    if (projects.size() > EditorSettings::k_max_recent_projects)
    {
        projects.resize(EditorSettings::k_max_recent_projects);
    }
}

bool EditorSettingsManager::load()
{
    return SettingsSerializer::read_editor_settings(editor_prefs_path(), m_settings);
}

bool EditorSettingsManager::save() const
{
    return SettingsSerializer::write_editor_settings(editor_prefs_path(), m_settings);
}

bool EditorSettingsManager::load_engine()
{
    EngineSettings s;
    if (!SettingsSerializer::read_engine_settings(engine_prefs_path(), s))
    {
        return false;
    }
    EngineSettingsManager::get().apply(s);
    return true;
}

bool EditorSettingsManager::save_engine() const
{
    return SettingsSerializer::write_engine_settings(engine_prefs_path(), EngineSettingsManager::get().settings());
}

} // namespace Ignis
