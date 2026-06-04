#include "edpch.h"
#include "EditorInputManager.h"

#include <Ignis/Input/InputSystem.h>

namespace Ignis
{

Vector<EditorInputManager::PanelMapping> EditorInputManager::s_mappings;

void EditorInputManager::init()
{
    s_mappings.clear();
}

void EditorInputManager::shutdown()
{
    s_mappings.clear();
}

void EditorInputManager::register_panel_context(InputContext* ctx, const char* panel_name)
{
    s_mappings.push_back({ctx, panel_name});
}

void EditorInputManager::begin_frame()
{
    ImGuiIO& io = ImGui::GetIO();
    InputSystem::set_block_game_input(io.WantCaptureKeyboard);

    for (auto& m : s_mappings)
    {
        m.ctx->set_active(false);
    }
}

void EditorInputManager::set_panel_active(const char* panel_name, bool active)
{
    for (auto& m : s_mappings)
    {
        if (strcmp(m.panel_name, panel_name) == 0)
        {
            m.ctx->set_active(active);
            return;
        }
    }
}

} // namespace Ignis
