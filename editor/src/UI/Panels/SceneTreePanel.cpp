#include "edpch.h"
#include "SceneTreePanel.h"

#include "../SceneEditor/SceneEditorContext.h"
#include "EditorPrimitives.h"
#include <Ignis/Scene/Scene.h>
#include <Ignis/Scene/Entity.h>
#include <Ignis/Scene/Components/Components.h>
#include <imgui.h>

namespace Ignis
{

static constexpr PanelId k_id = 1;

PanelId SceneTreePanel::get_id() const
{
    return k_id;
}

void SceneTreePanel::draw(IWorkspaceData* ctx)
{
    auto*   data            = static_cast<SceneEditorData*>(ctx);
    Scene&  scene           = *data->scene;
    Entity& selected_entity = *data->selected_entity;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {2.0f, 2.0f});
    if (ImGui::SmallButton("+"))
    {
        scene.create_entity("Entity");
    }
    ImGui::PopStyleVar();
    ImGui::SameLine();
    ImGui::TextDisabled("(%zu)", scene.registry().storage<entt::entity>().size());
    ImGui::Separator();

    for (auto e : scene.registry().storage<entt::entity>())
    {
        draw_entity_node(scene, e, selected_entity);
    }

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
    {
        selected_entity = Entity{};
    }

    if (selected_entity && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
    {
        scene.registry().destroy(static_cast<entt::entity>(selected_entity));
        selected_entity = Entity{};
    }

    if (ImGui::BeginPopupContextWindow("##SceneTreeCtx",
                                       ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Empty"))
        {
            scene.create_entity("Entity");
        }
        if (ImGui::BeginMenu("Create Primitive"))
        {
            if (ImGui::MenuItem("Cube"))
            {
                EditorPrimitives::spawn_cube(scene);
            }
            if (ImGui::MenuItem("Sphere"))
            {
                EditorPrimitives::spawn_sphere(scene);
            }
            if (ImGui::MenuItem("Plane"))
            {
                EditorPrimitives::spawn_quad(scene);
            }
            if (ImGui::MenuItem("Pyramid"))
            {
                EditorPrimitives::spawn_pyramid(scene);
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
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
