#include "edpch.h"

#ifdef ENGINE_IMGUI
#include "DockManager.h"

namespace Ignis
{

DockManager::DockManager(const PanelRegistry& registry)
    : m_registry(registry)
{
}

void DockManager::compile_layout(ImGuiID dockspace_id, const LayoutNode& root, ImVec2 size)
{
    m_dockspace_id = dockspace_id;

    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, size);

    compile_node(dockspace_id, root);

    ImGui::DockBuilderFinish(dockspace_id);
}

void DockManager::compile_node(ImGuiID node_id, const LayoutNode& node)
{
    if (node.is_leaf())
    {
        for (PanelId pid : node.panel_ids)
        {
            const PanelDescriptor* desc = m_registry.find(pid);
            if (desc)
            {
                ImGui::DockBuilderDockWindow(desc->title, node_id);
            }
        }

        ImGuiDockNode* dock_node = ImGui::DockBuilderGetNode(node_id);
        if (dock_node)
        {
            ImGuiDockNodeFlags flags = dock_node->LocalFlags | ImGuiDockNodeFlags_NoWindowMenuButton;
            if (node.hide_tab_bar)
            {
                flags |= ImGuiDockNodeFlags_NoTabBar;
            }
            dock_node->SetLocalFlags(flags);
        }
        return;
    }

    ImGuiID split_off = 0;
    ImGuiID remainder = 0;
    ImGui::DockBuilderSplitNode(node_id, to_imgui_dir(node.split_dir), node.ratio, &split_off, &remainder);

    if (node.child_a)
    {
        compile_node(split_off, *node.child_a);
    }
    if (node.child_b)
    {
        compile_node(remainder, *node.child_b);
    }
}

ImGuiDir DockManager::to_imgui_dir(SplitDir dir)
{
    switch (dir)
    {
        case SplitDir::Left:
            return ImGuiDir_Left;
        case SplitDir::Right:
            return ImGuiDir_Right;
        case SplitDir::Up:
            return ImGuiDir_Up;
        case SplitDir::Down:
            return ImGuiDir_Down;
    }
    return ImGuiDir_Left;
}

} // namespace Ignis
#endif
