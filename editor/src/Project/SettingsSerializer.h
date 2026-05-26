#pragma once
#include <Ignis.h>
#include "EditorSettings.h"
#include "Ignis/Settings/EngineSettings.h"

namespace Ignis
{

// TODO: move to the engine/settings module and use in the editor as well and then find away to also have a serializer
// here or have two serializers!
struct SettingsSerializer
{
    static bool read_editor_settings(const Path& path, EditorSettings& out);
    static bool write_editor_settings(const Path& path, const EditorSettings& settings);

    static bool read_engine_settings(const Path& path, EngineSettings& out);
    static bool write_engine_settings(const Path& path, const EngineSettings& settings);
};

} // namespace Ignis
