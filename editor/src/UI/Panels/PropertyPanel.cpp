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

static void draw_transform_component(TransformComponent& t)
{
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float pos[3] = {t.position.x, t.position.y, t.position.z};
        if (ImGui::DragFloat3("Position", pos, 0.1f))
        {
            t.position = {pos[0], pos[1], pos[2]};
        }

        float rot[4] = {t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w};
        if (ImGui::DragFloat4("Rotation (xyzw)", rot, 0.01f))
        {
            t.rotation = {rot[0], rot[1], rot[2], rot[3]};
        }

        float scl[3] = {t.scale.x, t.scale.y, t.scale.z};
        if (ImGui::DragFloat3("Scale", scl, 0.05f))
        {
            t.scale = {scl[0], scl[1], scl[2]};
        }
    }
}

static void draw_mesh_renderer_component(MeshRendererComponent& m)
{
    if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Visible", &m.is_visible);
        ImGui::LabelText("Mesh ID", "%llu", static_cast<uint64_t>(m.mesh_id));
    }
}

static void draw_material_component(MaterialComponent& m)
{
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::LabelText("Material ID", "%llu", static_cast<uint64_t>(m.material_id));
    }
}

void PropertyPanel::draw(IWorkspaceData* ctx)
{
    auto*   data     = static_cast<SceneEditorData*>(ctx);
    Scene&  scene    = *data->scene;
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

    if (selected.has_component<TransformComponent>())
    {
        draw_transform_component(selected.get_component<TransformComponent>());
    }
    if (selected.has_component<MeshRendererComponent>())
    {
        draw_mesh_renderer_component(selected.get_component<MeshRendererComponent>());
    }
    if (selected.has_component<MaterialComponent>())
    {
        draw_material_component(selected.get_component<MaterialComponent>());
    }
}

} // namespace Ignis
