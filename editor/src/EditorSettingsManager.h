#pragma once

#include <Ignis.h>
#include "EditorSettings.h"

namespace Ignis
{

class EditorSettingsManager
{
public:
    static EditorSettingsManager& get()
    {
        static EditorSettingsManager instance;
        return instance;
    }

    static Path editor_prefs_path();
    static Path engine_prefs_path();

    EditorSettings& settings()
    {
        return m_settings;
    }
    const EditorSettings& settings() const
    {
        return m_settings;
    }

    void push_recent(const Path& p);

    bool load();
    bool save() const;

    bool load_engine();
    bool save_engine() const;

private:
    EditorSettingsManager() = default;

    EditorSettings m_settings;
};

} // namespace Ignis
