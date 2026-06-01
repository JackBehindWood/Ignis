#pragma once
#include <Ignis/Input/InputContext.h>
#include <Ignis/Foundation/Foundation.h>

namespace Ignis
{

struct ImGuiShortcutString
{
    FixedInputString<32> value;

    // Implicitly converts to const char* when passed to ImGui
    constexpr operator const char*() const noexcept
    {
        return value.empty() ? nullptr : value.c_str();
    }
};

class EditorInputManager
{
public:
    static void init();
    static void shutdown();

    static void register_panel_context(InputContext* ctx, const char* panel_name);

    static void begin_frame();
    static void set_panel_active(const char* panel_name, bool active);

private:
    struct PanelMapping
    {
        InputContext* ctx;
        const char*   panel_name;
    };

    static Vector<PanelMapping> s_mappings;
};

} // namespace Ignis
