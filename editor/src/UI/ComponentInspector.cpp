#include "edpch.h"
#include "UI/ComponentInspector.h"
#include "UI/ImExt/ImExt.h"
#include "UI/Commands/SceneCommands.h"
#include "EditorResourceCache.h"
#include "Asset/EditorAssetManager.h"

#include <Ignis/Scene/Components/Components.h>
#include <Ignis/Scene/Components/CameraComponent.h>

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

namespace Utils
{

// ---------- Asset thumbnail helpers ----------

static ImTextureID imgui_tex(GRITexture2D* tex)
{
    return tex ? reinterpret_cast<ImTextureID>(tex->get_native_handle()) : ImTextureID{};
}

struct AssetPickResult
{
    bool    assigned = false;
    AssetID new_id{};
};

static AssetPickResult draw_asset_thumbnail_property(const char* label, AssetID current_id, const char* type_label,
                                                     AssetType asset_type, GRITexture2D* fallback_tex)
{
    AssetPickResult result;

    const ImVec2 thumb_size = {80.0f, 80.0f};

    Path src = current_id ? EditorAssetManager::get().try_get_source_path(current_id) : Path{};

    EditorResourceCache::ThumbnailResult tr{};
    if (current_id && !src.empty())
    {
        tr = EditorResourceCache::get().request_thumbnail(src, type_label);
    }

    if (tr.ready)
    {
        ImGui::Image(reinterpret_cast<ImTextureID>(tr.tex_id), thumb_size, {tr.uv0[0], tr.uv0[1]},
                     {tr.uv1[0], tr.uv1[1]});
    }
    else if (ImTextureID fb = imgui_tex(fallback_tex))
    {
        ImGui::Image(fb, thumb_size);
    }
    else
    {
        ImGui::Dummy(thumb_size);
        ImDrawList* dl   = ImGui::GetWindowDrawList();
        ImVec2      rmin = ImGui::GetItemRectMin();
        ImVec2      rmax = ImGui::GetItemRectMax();
        dl->AddRectFilled(rmin, rmax, IM_COL32(40, 40, 40, 255));
    }

    if (auto target = ImExt::DragDrop::Target<AssetDragPayload>())
    {
        if (const auto* hover = target.peek())
        {
            bool compat = hover->count > 0 && std::strcmp(hover->items[0].type_label, type_label) == 0;
            target.draw_compat_feedback(compat, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            ImGui::SetTooltip(compat ? "Drop to assign" : "Wrong type — needs %s", type_label);
        }
        if (const auto* p = target.accept())
        {
            if (p->count > 0 && std::strcmp(p->items[0].type_label, type_label) == 0)
            {
                auto& pm = ProjectManager::get();
                if (pm.is_open())
                {
                    Path abs        = pm.descriptor().root / pm.descriptor().asset_source_dir / p->items[0].rel_path;
                    result.new_id   = AssetManager::get().import(abs, asset_type);
                    result.assigned = true;
                }
            }
        }
    }

    ImGui::SameLine();
    ImGui::TextUnformatted(label);

    return result;
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
    auto& m   = e.add_component<MeshRendererComponent>();
    m.mesh_id = EditorResourceCache::get().get_fallback_mesh_id();
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

    ImGui::Checkbox("Visible", &m.is_visible);

    GRITexture2D* mesh_icon = EditorResourceCache::get().get_prim_thumbnail(static_cast<uint64_t>(m.mesh_id));
    if (!mesh_icon)
    {
        mesh_icon = EditorResourceCache::get().get_type_icon("MSH");
    }
    AssetPickResult r = draw_asset_thumbnail_property("Mesh", m.mesh_id, "MSH", AssetType::Mesh, mesh_icon);
    if (r.assigned)
    {
        if (auto* d = CommandDispatcher::active())
        {
            d->commit(create_unique<AssignMeshToEntityCmd>(e, m.mesh_id, r.new_id));
        }
        else
        {
            m.mesh_id = r.new_id;
        }
    }
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

    // ---- Material slot ----
    const ImVec2 thumb_size = {64.0f, 64.0f};

    Path mat_src = m.material_id ? EditorAssetManager::get().try_get_source_path(m.material_id) : Path{};

    EditorResourceCache::ThumbnailResult tr{};
    if (m.material_id && !mat_src.empty())
    {
        tr = EditorResourceCache::get().request_thumbnail(mat_src, "MAT");
    }

    if (tr.ready)
    {
        ImGui::Image(reinterpret_cast<ImTextureID>(tr.tex_id), thumb_size, {tr.uv0[0], tr.uv0[1]},
                     {tr.uv1[0], tr.uv1[1]});
    }
    else
    {
        ImGui::Dummy(thumb_size);
        ImDrawList* dl   = ImGui::GetWindowDrawList();
        ImVec2      rmin = ImGui::GetItemRectMin();
        ImVec2      rmax = ImGui::GetItemRectMax();
        dl->AddRectFilled(rmin, rmax, m.material_id ? IM_COL32(60, 60, 80, 255) : IM_COL32(35, 35, 35, 255));
        dl->AddRect(rmin, rmax, IM_COL32(80, 80, 80, 255));
    }

    if (auto target = ImExt::DragDrop::Target<AssetDragPayload>())
    {
        if (const auto* hover = target.peek())
        {
            bool compat = hover->count > 0 && std::strcmp(hover->items[0].type_label, "MAT") == 0;
            target.draw_compat_feedback(compat, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            ImGui::SetTooltip(compat ? "Drop to assign material" : "Wrong type — needs MAT");
        }
        if (const auto* p = target.accept())
        {
            if (p->count > 0 && std::strcmp(p->items[0].type_label, "MAT") == 0)
            {
                auto& pm = ProjectManager::get();
                if (pm.is_open())
                {
                    Path    abs    = pm.descriptor().root / pm.descriptor().asset_source_dir / p->items[0].rel_path;
                    AssetID new_id = AssetManager::get().import(abs, AssetType::Material);
                    if (auto* d = CommandDispatcher::active())
                    {
                        d->commit(create_unique<AssignMaterialToEntityCmd>(e, m.material_id, new_id));
                    }
                    else
                    {
                        m.material_id = new_id;
                    }
                }
            }
        }
    }

    ImGui::SameLine();
    ImGui::BeginGroup();
    {
        String mat_name = mat_src.empty() ? "None" : mat_src.stem().string();
        ImGui::TextUnformatted(mat_name.c_str());

        if (m.material_id)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
            char id_buf[24];
            std::snprintf(id_buf, sizeof(id_buf), "#%llu", static_cast<uint64_t>(m.material_id));
            ImGui::TextUnformatted(id_buf);
            ImGui::PopStyleColor();

            if (ImGui::SmallButton("Clear"))
            {
                if (auto* d = CommandDispatcher::active())
                {
                    d->commit(create_unique<AssignMaterialToEntityCmd>(e, m.material_id, AssetID{}));
                }
                else
                {
                    m.material_id = AssetID{};
                }
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 100, 100, 255));
            ImGui::TextUnformatted("Drag a .mat asset here");
            ImGui::PopStyleColor();
        }
    }
    ImGui::EndGroup();

    const UUID entity_uuid = e.get_component<IDComponent>().id;

    if (!m.material_id)
    {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 180, 120, 255));
        ImGui::TextUnformatted("PBR (inline)");
        ImGui::PopStyleColor();

        bool dirty = false;
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.65f);
        dirty |= ImGui::ColorEdit4("Albedo", m.albedo_colour.data, ImGuiColorEditFlags_AlphaBar);
        dirty |= ImGui::DragFloat("Alpha Cutoff", &m.alpha_cutoff, 0.01f, 0.0f, 1.0f);
        dirty |= ImGui::DragFloat("Emissive Intensity", &m.emissive_intensity, 0.1f, 0.0f, 100.0f);
        ImGui::PopItemWidth();
        if (dirty)
        {
            m.params_dirty = true;
        }
    }

    ImGui::Spacing();
    if (ImGui::SmallButton("Edit Material..."))
    {
        if (auto* d = CommandDispatcher::active())
        {
            d->enqueue(create_unique<OpenMaterialEditorCmd>(entity_uuid));
        }
    }
}

// ---------- Texture ----------

static bool has_texture(Entity& e)
{
    return e.has_component<TextureComponent>();
}
static void add_texture(Entity& e)
{
    auto& t      = e.add_component<TextureComponent>();
    t.texture_id = AssetID(UUID::s_invalid);
}
static void remove_texture(Entity& e)
{
    e.remove_component<TextureComponent>();
}
static void draw_texture(Entity& e)
{
    if (!ImGui::CollapsingHeader("Texture", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    auto& t = e.get_component<TextureComponent>();

    auto r = draw_asset_thumbnail_property("Texture", t.texture_id, "TEX", AssetType::Texture2D,
                                           EditorResourceCache::get().get_white_texture());

    if (r.assigned)
    {
        if (auto* d = CommandDispatcher::active())
        {
            d->commit(create_unique<AssignTextureToEntityCmd>(e, t.texture_id, r.new_id));
        }
        else
        {
            t.texture_id = r.new_id;
        }
    }
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

// ---------- Directional Light ----------

static bool has_directional_light(Entity& e)
{
    return e.has_component<DirectionalLightComponent>();
}
static void add_directional_light(Entity& e)
{
    e.add_component<DirectionalLightComponent>();
}
static void remove_directional_light(Entity& e)
{
    e.remove_component<DirectionalLightComponent>();
}
static void draw_directional_light(Entity& e)
{
    if (!ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    auto& light = e.get_component<DirectionalLightComponent>();

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 72.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.5f);

    draw_field("Color", e, light.color,
               [](Entity& ent, const Math::Vec3f& v) { ent.get_component<DirectionalLightComponent>().color = v; });

    draw_field(
        "Intensity", e, light.intensity,
        [](Entity& ent, const float& v) { ent.get_component<DirectionalLightComponent>().intensity = v; }, 0.05f);

    draw_field("Direction", e, light.direction,
               [](Entity& ent, const Math::Vec3f& v) { ent.get_component<DirectionalLightComponent>().direction = v; });

    ImGui::PopStyleVar();
    ImGui::PopItemWidth();
}

// ---------- Point Light ----------

static bool has_point_light(Entity& e)
{
    return e.has_component<PointLightComponent>();
}
static void add_point_light(Entity& e)
{
    e.add_component<PointLightComponent>();
}
static void remove_point_light(Entity& e)
{
    e.remove_component<PointLightComponent>();
}
static void draw_point_light(Entity& e)
{
    if (!ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    auto& light = e.get_component<PointLightComponent>();

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 72.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.5f);

    draw_field("Color", e, light.color,
               [](Entity& ent, const Math::Vec3f& v) { ent.get_component<PointLightComponent>().color = v; });

    draw_field(
        "Intensity", e, light.intensity,
        [](Entity& ent, const float& v) { ent.get_component<PointLightComponent>().intensity = v; }, 0.05f);

    draw_field(
        "Radius", e, light.radius,
        [](Entity& ent, const float& v) { ent.get_component<PointLightComponent>().radius = v; }, 0.1f);

    ImGui::PopStyleVar();
    ImGui::PopItemWidth();
}

// ---------- Spot Light ----------

static bool has_spot_light(Entity& e)
{
    return e.has_component<SpotLightComponent>();
}
static void add_spot_light(Entity& e)
{
    e.add_component<SpotLightComponent>();
}
static void remove_spot_light(Entity& e)
{
    e.remove_component<SpotLightComponent>();
}
static void draw_spot_light(Entity& e)
{
    if (!ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    auto& light = e.get_component<SpotLightComponent>();

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 72.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.5f);

    draw_field("Color", e, light.color,
               [](Entity& ent, const Math::Vec3f& v) { ent.get_component<SpotLightComponent>().color = v; });

    draw_field(
        "Intensity", e, light.intensity,
        [](Entity& ent, const float& v) { ent.get_component<SpotLightComponent>().intensity = v; }, 0.05f);

    draw_field(
        "Radius", e, light.radius,
        [](Entity& ent, const float& v) { ent.get_component<SpotLightComponent>().radius = v; }, 0.1f);

    draw_field("Direction", e, light.direction,
               [](Entity& ent, const Math::Vec3f& v) { ent.get_component<SpotLightComponent>().direction = v; });

    draw_field(
        "Inner Angle", e, light.inner_cone_angle,
        [](Entity& ent, const float& v) { ent.get_component<SpotLightComponent>().inner_cone_angle = v; }, 0.5f);

    draw_field(
        "Outer Angle", e, light.outer_cone_angle,
        [](Entity& ent, const float& v) { ent.get_component<SpotLightComponent>().outer_cone_angle = v; }, 0.5f);

    ImGui::PopStyleVar();
    ImGui::PopItemWidth();
}

} // namespace Utils

// ---------- Registration ----------

void ComponentInspector::register_defaults()
{
    register_component(
        {"Transform", "", Utils::has_transform, Utils::add_transform, Utils::remove_transform, Utils::draw_transform});
    register_component({"Mesh Renderer", "Rendering", Utils::has_mesh_renderer, Utils::add_mesh_renderer,
                        Utils::remove_mesh_renderer, Utils::draw_mesh_renderer});
    register_component({"Material", "Rendering", Utils::has_material, Utils::add_material, Utils::remove_material,
                        Utils::draw_material});
    register_component(
        {"Texture", "Rendering", Utils::has_texture, Utils::add_texture, Utils::remove_texture, Utils::draw_texture});
    register_component(
        {"Camera", "Scene", Utils::has_camera, Utils::add_camera, Utils::remove_camera, Utils::draw_camera});

    // Light Components Registration
    register_component({"Directional Light", "Lighting", Utils::has_directional_light, Utils::add_directional_light,
                        Utils::remove_directional_light, Utils::draw_directional_light});
    register_component({"Point Light", "Lighting", Utils::has_point_light, Utils::add_point_light,
                        Utils::remove_point_light, Utils::draw_point_light});
    register_component({"Spot Light", "Lighting", Utils::has_spot_light, Utils::add_spot_light,
                        Utils::remove_spot_light, Utils::draw_spot_light});
}

} // namespace Ignis
