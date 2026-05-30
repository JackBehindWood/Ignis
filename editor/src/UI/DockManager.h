#pragma once

#ifdef ENGINE_IMGUI
#include <imgui.h>
#include "Workspace/LayoutNode.h"
#include "Panels/PanelRegistry.h"

namespace Ignis
{

class DockManager
{
public:
    explicit DockManager(const PanelRegistry& registry);

    // Rebuilds the full DockBuilder tree from a declarative LayoutNode.
    // Must be called on layout-dirty frames only, after ImGui::DockSpace()
    // and before any panel Begin() calls for that frame.
    void compile_layout(ImGuiID dockspace_id, const LayoutNode& root, ImVec2 size);

    ImGuiID dockspace_id() const
    {
        return m_dockspace_id;
    }

private:
    void            compile_node(ImGuiID node_id, const LayoutNode& node);
    static ImGuiDir to_imgui_dir(SplitDir dir);

    const PanelRegistry& m_registry;
    ImGuiID              m_dockspace_id = 0;
};

} // namespace Ignis
#endif
