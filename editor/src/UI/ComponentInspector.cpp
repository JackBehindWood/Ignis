#include "edpch.h"
#include "UI/ComponentInspector.h"

#include <Ignis/Scene/Components/Components.h>
#include <Ignis/Scene/Components/CameraComponent.h>
#include <imgui.h>

namespace Ignis
{

static Vector<ComponentDescriptor> s_descriptors;

void ComponentInspector::register_component(ComponentDescriptor desc)
{
    s_descriptors.push_back(desc);
}

const Vector<ComponentDescriptor>& ComponentInspector::all()
{
    return s_descriptors;
}

// ---------- Transform ----------

static bool has_transform(Entity& e)
{
    return e.has_component<TransformComponent>();
}
static void add_transform(Entity& e)
{
    e.add_component<TransformComponent>();
}
static void draw_transform(Entity& e)
{
    if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    auto& t = e.get_component<TransformComponent>();

    float pos[3] = {t.position.x, t.position.y, t.position.z};
    if (ImGui::DragFloat3("Position", pos, 0.1f))
    {
        t.position = {pos[0], pos[1], pos[2]};
    }

    static Math::Vec3f         s_euler_display   = {};
    static bool                s_rotation_active = false;
    static TransformComponent* s_euler_owner     = nullptr;

    if (&t != s_euler_owner)
    {
        s_euler_owner     = &t;
        s_rotation_active = false;
    }
    if (!s_rotation_active)
    {
        s_euler_display = t.rotation.to_euler();
    }

    float rot[3] = {Math::degrees(s_euler_display.x), Math::degrees(s_euler_display.y),
                    Math::degrees(s_euler_display.z)};
    if (ImGui::DragFloat3("Rotation", rot, 0.5f))
    {
        s_euler_display = {Math::radians(rot[0]), Math::radians(rot[1]), Math::radians(rot[2])};
        t.rotation      = Math::Quatf::from_euler(s_euler_display).normalized();
    }
    s_rotation_active = ImGui::IsItemActive();

    float scl[3] = {t.scale.x, t.scale.y, t.scale.z};
    if (ImGui::DragFloat3("Scale", scl, 0.05f))
    {
        t.scale = {scl[0], scl[1], scl[2]};
    }
}

// ---------- MeshRenderer ----------

static bool has_mesh_renderer(Entity& e)
{
    return e.has_component<MeshRendererComponent>();
}
static void add_mesh_renderer(Entity& e)
{
    e.add_component<MeshRendererComponent>();
}
static void draw_mesh_renderer(Entity& e)
{
    if (!ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    auto& m = e.get_component<MeshRendererComponent>();
    ImGui::Checkbox("Visible", &m.is_visible);
    ImGui::LabelText("Mesh ID", "%llu", static_cast<uint64_t>(m.mesh_id));
}

// ---------- Material ----------

static bool has_material(Entity& e)
{
    return e.has_component<MaterialComponent>();
}
static void add_material(Entity& e)
{
    e.add_component<MaterialComponent>();
}
static void draw_material(Entity& e)
{
    if (!ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    auto& m = e.get_component<MaterialComponent>();
    ImGui::LabelText("Material ID", "%llu", static_cast<uint64_t>(m.material_id));
}

// ---------- Camera ----------

static bool has_camera(Entity& e)
{
    return e.has_component<CameraComponent>();
}
static void add_camera(Entity& e)
{
    e.add_component<CameraComponent>();
}
static void draw_camera(Entity& e)
{
    if (!ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    auto& cc = e.get_component<CameraComponent>();
    ImGui::Checkbox("Primary", &cc.is_primary);

    const bool is_persp = cc.camera.projection_type() == SceneCamera::ProjectionType::Perspective;
    ImGui::Text("Projection: %s", is_persp ? "Perspective" : "Orthographic");

    if (is_persp)
    {
        float fov = cc.camera.fov();
        if (ImGui::DragFloat("FOV", &fov, 0.5f, 1.0f, 175.0f))
        {
            cc.camera.set_perspective(fov, cc.camera.near_clip(), cc.camera.far_clip());
        }
    }

    float near_clip = cc.camera.near_clip();
    float far_clip  = cc.camera.far_clip();
    if (ImGui::DragFloat("Near", &near_clip, 0.01f, 0.001f, far_clip - 0.01f))
    {
        cc.camera.set_perspective(cc.camera.fov(), near_clip, cc.camera.far_clip());
    }
    if (ImGui::DragFloat("Far", &far_clip, 1.0f, near_clip + 0.01f, 100000.0f))
    {
        cc.camera.set_perspective(cc.camera.fov(), cc.camera.near_clip(), far_clip);
    }
}

// ---------- Registration ----------

void ComponentInspector::register_defaults()
{
    register_component({"Transform", has_transform, add_transform, draw_transform});
    register_component({"Mesh Renderer", has_mesh_renderer, add_mesh_renderer, draw_mesh_renderer});
    register_component({"Material", has_material, add_material, draw_material});
    register_component({"Camera", has_camera, add_camera, draw_camera});
}

} // namespace Ignis
