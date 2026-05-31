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
    SceneEditorData* data            = static_cast<SceneEditorData*>(ctx);
    Scene&           scene           = *data->scene;
    Entity&          selected_entity = *data->selected_entity;

    float counter_width    = ImGui::CalcTextSize("(9999)").x;
    float add_button_width = ImGui::GetContentRegionAvail().x - counter_width - ImGui::GetStyle().ItemSpacing.x;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {4.0f, 3.0f});
    if (ImGui::Button("Add Entity...", {add_button_width, 0.0f}))
    {
        ImGui::OpenPopup("AddEntityPopup");
    }
    ImGui::PopStyleVar();

    if (ImGui::BeginPopup("AddEntityPopup"))
    {
        if (ImGui::MenuItem("Create Empty"))
        {
            if (data->dispatcher)
            {
                data->dispatcher->commit(create_unique<CreateEntityCmd>(&scene, "Entity", &selected_entity));
            }
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

    ImGui::SameLine();
    float right_edge_counter = ImGui::GetWindowWidth() - counter_width - ImGui::GetStyle().WindowPadding.x;
    ImGui::SetCursorPosX(right_edge_counter);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("(%zu)", scene.registry().storage<entt::entity>().size());

    float clear_btn_width = ImGui::CalcTextSize("Clear").x + (ImGui::GetStyle().FramePadding.x * 2.0f);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - clear_btn_width - ImGui::GetStyle().ItemSpacing.x);
    ImGui::InputText("##Search", m_search_buffer, sizeof(m_search_buffer));

    ImGui::SameLine();
    if (ImGui::Button("Clear") || ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        m_search_buffer[0] = '\0';
    }

    ImGui::Separator();

    String search_query(m_search_buffer);
    bool   has_filter = !search_query.empty();

    for (auto e : scene.registry().storage<entt::entity>())
    {
        if (has_filter)
        {
            Entity      entity{e, &scene};
            const char* name = "Entity";
            if (entity.has_component<NameComponent>())
            {
                name = entity.get_component<NameComponent>().name.c_str();
            }
            if (!str_icontains(name, search_query.c_str()))
            {
                continue;
            }
        }

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
            if (data->dispatcher)
            {
                data->dispatcher->commit(create_unique<CreateEntityCmd>(&scene, "Entity", &selected_entity));
            }
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
