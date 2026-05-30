#include "edpch.h"
#include "PropertyPanel.h"

#include "../SceneEditor/SceneEditorContext.h"
#include <Ignis/Scene/Components/Components.h>
#include <imgui.h>

namespace Ignis
{

static constexpr PanelId k_id = 4;

PanelId PropertyPanel::get_id() const
{
    return k_id;
}

void PropertyPanel::draw(IWorkspaceData* ctx)
{
    auto*   data     = static_cast<SceneEditorData*>(ctx);
    Entity& selected = *data->selected_entity;

    if (!selected)
    {
        ImGui::TextDisabled("No entity selected");
        return;
    }

    if (selected.has_component<NameComponent>())
    {
        auto& name = selected.get_component<NameComponent>().name;
        char  buf[256];
        std::strncpy(buf, name.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        if (ImGui::InputText("Name", buf, sizeof(buf)))
        {
            name = buf;
        }
    }

    ImGui::Separator();

    for (const auto& desc : ComponentInspector::all())
    {
        if (desc.has(selected))
        {
            desc.draw(selected);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const float btn_w = ImGui::GetContentRegionAvail().x;
    if (ImGui::Button("Add Component", {btn_w, 0.0f}))
    {
        ImGui::OpenPopup("##AddComponent");
    }

    if (ImGui::BeginPopup("##AddComponent"))
    {
        bool any = false;
        for (const auto& desc : ComponentInspector::all())
        {
            if (!desc.has(selected))
            {
                any = true;
                if (ImGui::MenuItem(desc.name))
                {
                    desc.add(selected);
                }
            }
        }
        if (!any)
        {
            ImGui::TextDisabled("All components attached");
        }
        ImGui::EndPopup();
    }
}

} // namespace Ignis
