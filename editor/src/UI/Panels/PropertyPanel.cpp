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
    SceneEditorData* data     = static_cast<SceneEditorData*>(ctx);
    Entity&          selected = *data->selected_entity;

    if (!selected)
    {
        ImGui::TextDisabled("No entity selected");
        return;
    }

    if (selected.has_component<NameComponent>())
    {
        String& name = selected.get_component<NameComponent>().name;
        char    buf[256];
        std::strncpy(buf, name.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        const String before  = name;
        if (ImGui::InputText("Name", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (data->dispatcher)
            {
                data->dispatcher->commit(create_unique<ChangeNameCmd>(selected, before, String(buf)));
            }
        }
    }

    ImGui::Separator();

    CommandDispatcher::set_active(data->dispatcher);
    for (const auto& desc : ComponentInspector::all())
    {
        if (desc.has(selected))
        {
            desc.draw(selected);
        }
    }
    CommandDispatcher::set_active(nullptr);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const float btn_w = ImGui::GetContentRegionAvail().x;
    if (ImGui::Button("Add Component", {btn_w, 0.0f}))
    {
        ImGui::OpenPopup("##AddComponent");
    }

    draw_add_component_popup(data, selected);
}

void PropertyPanel::draw_add_component_popup(SceneEditorData* data, Entity& selected)
{
    ImGui::SetNextWindowSize({280.0f, 0.0f});
    if (ImGui::BeginPopup("##AddComponent"))
    {
        ImGui::TextDisabled("Search:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##CompSearch", m_component_search, sizeof(m_component_search));

        ImGui::Separator();
        ImGui::Spacing();

        auto draw_popup_item = [&](const ComponentDescriptor& desc)
        {
            if (ImGui::MenuItem(desc.name))
            {
                if (data->dispatcher)
                {
                    data->dispatcher->commit(create_unique<AddComponentCmd>(selected, desc.add, desc.remove));
                }
                else
                {
                    desc.add(selected);
                }
                m_component_search[0] = '\0';
            }
        };

        bool attached_all = true;

        Vector<ComponentDescriptor>                       ungrouped;
        Vector<String>                                    category_order;
        UnorderedMap<String, Vector<ComponentDescriptor>> grouped;

        for (const ComponentDescriptor& desc : ComponentInspector::all())
        {
            if (!desc.has(selected))
            {
                attached_all = false;

                bool name_matches     = str_icontains(desc.name, m_component_search);
                bool category_matches = desc.category[0] != '\0' && str_icontains(desc.category, m_component_search);

                if (!name_matches && !category_matches)
                {
                    continue;
                }

                if (desc.category[0] == '\0')
                {
                    ungrouped.push_back(desc);
                }
                else
                {
                    String cat(desc.category);
                    if (grouped.find(cat) == grouped.end())
                    {
                        category_order.push_back(cat);
                    }
                    grouped[cat].push_back(desc);
                }
            }
        }

        if (attached_all)
        {
            ImGui::TextDisabled("All components attached");
        }
        else
        {
            for (ComponentDescriptor& d : ungrouped)
            {
                draw_popup_item(d);
            }
            if (!ungrouped.empty() && !category_order.empty())
            {
                ImGui::Separator();
            }

            for (const auto& cat : category_order)
            {
                if (ImGui::BeginMenu(cat.c_str()))
                {
                    for (auto& d : grouped[cat])
                    {
                        draw_popup_item(d);
                    }
                    ImGui::EndMenu();
                }
            }
        }

        ImGui::EndPopup();
    }
}

} // namespace Ignis
