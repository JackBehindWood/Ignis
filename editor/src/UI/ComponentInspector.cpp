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

// ---------- Field widget specialisations ----------

template <typename T>
struct FieldWidget;

template <>
struct FieldWidget<Math::Vec3f>
{
    static bool draw(const char* label, Math::Vec3f& v, float speed)
    {
        float f[3] = {v.x, v.y, v.z};
        if (!ImGui::DragFloat3(label, f, speed))
        {
            return false;
        }
        v = {f[0], f[1], f[2]};
        return true;
    }
};

template <>
struct FieldWidget<float>
{
    static bool draw(const char* label, float& v, float speed)
    {
        return ImGui::DragFloat(label, &v, speed);
    }
};

// per-ImGui-item before-state storage, keyed by item ID
template <typename T>
static UnorderedMap<ImGuiID, T>& item_before_map()
{
    static UnorderedMap<ImGuiID, T> s_map;
    return s_map;
}

// Draws a labelled drag widget and commits a PropertyChangeCmd when the
// interaction ends, using CommandDispatcher::active() as the sink.
template <typename T, typename Setter>
static void draw_field(const char* label, Entity entity, T& value, Setter&& setter, float speed = 0.1f)
{
    const T saved = value;
    FieldWidget<T>::draw(label, value, speed);
    const ImGuiID id = ImGui::GetItemID();
    if (ImGui::IsItemActivated())
    {
        item_before_map<T>()[id] = saved;
    }
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        if (CommandDispatcher* d = CommandDispatcher::active())
        {
            d->commit(make_property_cmd(entity, item_before_map<T>()[id], value, std::forward<Setter>(setter)));
        }
    }
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
static void remove_transform(Entity& e)
{
    e.remove_component<TransformComponent>();
}
static void draw_transform(Entity& e)
{
    if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    TransformComponent& t = e.get_component<TransformComponent>();

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 72.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.5f);

    draw_field("Position", e, t.position,
               [](Entity& ent, const Math::Vec3f& v) { ent.get_component<TransformComponent>().position = v; });

    {
        static UnorderedMap<ImGuiID, Math::Quatf> s_rot_before;
        static Math::Vec3f                        s_euler_display = {};
        static bool                               s_editing       = false;
        static TransformComponent*                s_owner         = nullptr;

        if (&t != s_owner)
        {
            s_owner   = &t;
            s_editing = false;
        }
        if (!s_editing)
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
        const ImGuiID id = ImGui::GetItemID();
        if (ImGui::IsItemActivated())
        {
            s_rot_before[id] = t.rotation;
        }
        s_editing = ImGui::IsItemActive();
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            if (auto* d = CommandDispatcher::active())
            {
                d->commit(make_property_cmd(e, s_rot_before[id], t.rotation, [](Entity& ent, const Math::Quatf& q)
                                            { ent.get_component<TransformComponent>().rotation = q; }));
            }
        }
    }

    draw_field(
        "Scale", e, t.scale,
        [](Entity& ent, const Math::Vec3f& v) { ent.get_component<TransformComponent>().scale = v; }, 0.05f);

    ImGui::PopStyleVar();
    ImGui::PopItemWidth();
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
static void remove_mesh_renderer(Entity& e)
{
    e.remove_component<MeshRendererComponent>();
}
static void draw_mesh_renderer(Entity& e)
{
    if (!ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    auto& m = e.get_component<MeshRendererComponent>();

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 72.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.5f);

    ImGui::Checkbox("Visible", &m.is_visible);
    ImGui::LabelText("Mesh ID", "%llu", static_cast<uint64_t>(m.mesh_id));

    ImGui::PopStyleVar();
    ImGui::PopItemWidth();
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
static void remove_material(Entity& e)
{
    e.remove_component<MaterialComponent>();
}
static void draw_material(Entity& e)
{
    if (!ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    auto& m = e.get_component<MaterialComponent>();

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 72.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.5f);

    ImGui::LabelText("Material ID", "%llu", static_cast<uint64_t>(m.material_id));

    ImGui::PopStyleVar();
    ImGui::PopItemWidth();
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
static void remove_camera(Entity& e)
{
    e.remove_component<CameraComponent>();
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

    auto commit_float = [&]<typename Setter>(ImGuiID id, float after, Setter&& setter)
    {
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            if (auto* d = CommandDispatcher::active())
            {
                d->commit(make_property_cmd(e, item_before_map<float>()[id], after, std::forward<Setter>(setter)));
            }
        }
    };

    if (is_persp)
    {
        const float fov_saved = cc.camera.fov();
        float       fov       = fov_saved;
        if (ImGui::DragFloat("FOV", &fov, 0.5f, 1.0f, 175.0f))
        {
            cc.camera.set_perspective(fov, cc.camera.near_clip(), cc.camera.far_clip());
        }
        const ImGuiID fov_id = ImGui::GetItemID();
        if (ImGui::IsItemActivated())
        {
            item_before_map<float>()[fov_id] = fov_saved;
        }
        commit_float(fov_id, cc.camera.fov(),
                     [](Entity& ent, const float& v)
                     {
                         auto& c = ent.get_component<CameraComponent>();
                         c.camera.set_perspective(v, c.camera.near_clip(), c.camera.far_clip());
                     });
    }

    {
        const float near_saved = cc.camera.near_clip();
        float       near_clip  = near_saved;
        if (ImGui::DragFloat("Near", &near_clip, 0.01f, 0.001f, cc.camera.far_clip() - 0.01f))
        {
            cc.camera.set_perspective(cc.camera.fov(), near_clip, cc.camera.far_clip());
        }
        const ImGuiID near_id = ImGui::GetItemID();
        if (ImGui::IsItemActivated())
        {
            item_before_map<float>()[near_id] = near_saved;
        }
        commit_float(near_id, cc.camera.near_clip(),
                     [](Entity& ent, const float& v)
                     {
                         auto& c = ent.get_component<CameraComponent>();
                         c.camera.set_perspective(c.camera.fov(), v, c.camera.far_clip());
                     });
    }

    {
        const float far_saved = cc.camera.far_clip();
        float       far_clip  = far_saved;
        if (ImGui::DragFloat("Far", &far_clip, 1.0f, cc.camera.near_clip() + 0.01f, 100000.0f))
        {
            cc.camera.set_perspective(cc.camera.fov(), cc.camera.near_clip(), far_clip);
        }
        const ImGuiID far_id = ImGui::GetItemID();
        if (ImGui::IsItemActivated())
        {
            item_before_map<float>()[far_id] = far_saved;
        }
        commit_float(far_id, cc.camera.far_clip(),
                     [](Entity& ent, const float& v)
                     {
                         auto& c = ent.get_component<CameraComponent>();
                         c.camera.set_perspective(c.camera.fov(), c.camera.near_clip(), v);
                     });
    }
}

// ---------- Registration ----------

void ComponentInspector::register_defaults()
{
    register_component({"Transform", "", has_transform, add_transform, remove_transform, draw_transform});
    register_component(
        {"Mesh Renderer", "Rendering", has_mesh_renderer, add_mesh_renderer, remove_mesh_renderer, draw_mesh_renderer});
    register_component({"Material", "Rendering", has_material, add_material, remove_material, draw_material});
    register_component({"Camera", "Scene", has_camera, add_camera, remove_camera, draw_camera});
}

} // namespace Ignis
