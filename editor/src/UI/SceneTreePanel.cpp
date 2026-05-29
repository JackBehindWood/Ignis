#include "edpch.h"
#include "SceneTreePanel.h"

#include <Ignis/Scene/Components/Components.h>
#include <imgui.h>

namespace Ignis
{

void SceneTreePanel::draw(Scene& scene, Entity& selected_entity)
{
    ImGui::Begin("Scene Tree");

    for (auto e : scene.registry().storage<entt::entity>())
    {
        draw_entity_node(scene, e, selected_entity);
    }

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
    {
        selected_entity = Entity{};
    }

    if (ImGui::BeginPopupContextWindow("##SceneTreeCtx",
                                       ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Entity"))
        {
            scene.create_entity();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void SceneTreePanel::draw_entity_node(Scene& scene, entt::entity e, Entity& selected_entity)
{
    Entity entity{e, &scene};

    String label = "Entity##" + std::to_string(static_cast<uint32_t>(e));
    if (entity.has_component<NameComponent>())
    {
        label = entity.get_component<NameComponent>().name + "##" + std::to_string(static_cast<uint32_t>(e));
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selected_entity == entity)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool open = ImGui::TreeNodeEx(label.c_str(), flags);

    if (ImGui::IsItemClicked())
    {
        selected_entity = entity;
    }

    bool destroy = false;
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Destroy Entity"))
        {
            destroy = true;
        }
        ImGui::EndPopup();
    }

    if (open)
    {
        ImGui::TreePop();
    }

    if (destroy)
    {
        if (selected_entity == entity)
        {
            selected_entity = Entity{};
        }
        scene.registry().destroy(e);
    }
}

} // namespace Ignis
