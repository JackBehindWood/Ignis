#include "edpch.h"
#include "MaterialEditorPanel.h"

#include "UI/SceneEditor/SceneEditorContext.h"
#include "UI/Commands/SceneCommands.h"
#include "UI/ImExt/ImExt.h"
#include "EditorResourceCache.h"
#include "Asset/EditorAssetManager.h"
#include "Project/ProjectManager.h"

#include <Ignis/Scene/Components/Components.h>
#include <Ignis/Rendering/Renderer.h>
#include <Ignis/Rendering/RenderSystem.h>
#include <Ignis/Rendering/PBRMaterialParams.h>
#include <Ignis/Rendering/RenderTexture2D.h>
#include <Ignis/Asset/AssetManager.h>

namespace Ignis
{

static constexpr uint64_t k_inline_mat_bit = 0x8000'0000'0000'0000ULL;

PanelId MaterialEditorPanel::get_id() const
{
    return 8;
}

void MaterialEditorPanel::update(float, IWorkspaceData* ctx)
{
    if (!ctx)
    {
        return;
    }
    auto* data  = static_cast<SceneEditorData*>(ctx);
    m_entity_id = data->material_editor_target;
}

bool MaterialEditorPanel::draw_pbr_texture_slot(const char* label, uint32_t& tex_idx_inout, AssetID* asset_id_inout,
                                                GRITexture2D* default_preview)
{
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    const ImVec2 thumb_size = {48.0f, 48.0f};

    ImTextureID preview_id =
        default_preview ? reinterpret_cast<ImTextureID>(default_preview->get_native_handle()) : ImTextureID{};

    if (preview_id)
    {
        ImGui::Image(preview_id, thumb_size);
    }
    else
    {
        ImGui::Dummy(thumb_size);
    }
    ImVec2      thumb_min = ImGui::GetItemRectMin();
    ImVec2      thumb_max = ImGui::GetItemRectMax();
    ImDrawList* dl        = ImGui::GetWindowDrawList();
    if (!preview_id)
    {
        ImU32 fill = tex_idx_inout ? IM_COL32(55, 75, 95, 255) : IM_COL32(35, 35, 35, 255);
        dl->AddRectFilled(thumb_min, thumb_max, fill);
    }
    dl->AddRect(thumb_min, thumb_max, IM_COL32(80, 80, 80, 255));

    bool changed = false;
    if (auto target = ImExt::DragDrop::Target<AssetDragPayload>())
    {
        if (const auto* hover = target.peek())
        {
            bool compat = hover->count > 0 && std::strcmp(hover->items[0].type_label, "TEX") == 0;
            target.draw_compat_feedback(compat, thumb_min, thumb_max);
            ImGui::SetTooltip(compat ? "Drop to assign texture" : "Wrong type — needs TEX");
        }
        if (const auto* p = target.accept())
        {
            if (p->count > 0 && std::strcmp(p->items[0].type_label, "TEX") == 0)
            {
                auto& pm = ProjectManager::get();
                if (pm.is_open())
                {
                    Path    abs    = pm.descriptor().root / pm.descriptor().asset_source_dir / p->items[0].rel_path;
                    AssetID tex_id = AssetManager::get().import(abs, AssetType::Texture2D);
                    SharedPtr<RenderTexture2D> rt =
                        Renderer::get_resource_cache().find_texture(static_cast<uint64_t>(tex_id));
                    if (rt)
                    {
                        GRISamplerDesc sd;
                        sd.linear_filter           = true;
                        GRISamplerStatePtr sampler = RenderSystem::get_gri()->create_sampler_state(sd);
                        tex_idx_inout = RenderSystem::get_gri()->register_bindless_texture(rt->get_texture_ptr(),
                                                                                           std::move(sampler));
                        if (asset_id_inout)
                        {
                            *asset_id_inout = tex_id;
                        }
                        changed = true;
                    }
                }
            }
        }
    }

    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(label);
    if (tex_idx_inout)
    {
        char slot_label[32];
        std::snprintf(slot_label, sizeof(slot_label), "Slot %u", tex_idx_inout);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120, 200, 120, 255));
        ImGui::TextUnformatted(slot_label);
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 100, 100, 255));
        ImGui::TextUnformatted("—");
        ImGui::PopStyleColor();
    }

    ImGui::TableSetColumnIndex(2);
    if (tex_idx_inout)
    {
        ImGui::PushID(label);
        if (ImGui::SmallButton("×"))
        {
            tex_idx_inout = 0;
            if (asset_id_inout)
            {
                *asset_id_inout = AssetID{};
            }
            changed = true;
        }
        ImGui::PopID();
    }

    return changed;
}

void MaterialEditorPanel::draw(IWorkspaceData* ctx)
{
    if (static_cast<uint64_t>(m_entity_id) == UUID::s_invalid)
    {
        ImGui::TextDisabled("No material selected");
        return;
    }

    if (!ctx)
    {
        return;
    }
    auto* data = static_cast<SceneEditorData*>(ctx);
    if (!data->scene)
    {
        ImGui::TextDisabled("No material selected");
        return;
    }

    Entity e = data->scene->find_entity(m_entity_id);
    if (!e.is_valid() || !e.has_component<MaterialComponent>())
    {
        ImGui::TextDisabled("No material selected");
        return;
    }

    MaterialComponent& mc = e.get_component<MaterialComponent>();

    const bool     is_inline = !mc.material_id;
    const uint64_t mat_key =
        is_inline ? (k_inline_mat_bit | static_cast<uint64_t>(m_entity_id)) : static_cast<uint64_t>(mc.material_id);

    SharedPtr<Material> mat = Renderer::get_resource_cache().find_material(mat_key);
    if (!mat || !mat->get_params_buffer())
    {
        ImGui::TextDisabled("Not a PBR material");
        return;
    }

    if (is_inline && e.has_component<NameComponent>())
    {
        ImGui::TextUnformatted(e.get_component<NameComponent>().name.c_str());
    }
    else if (!is_inline)
    {
        Path src = EditorAssetManager::get().try_get_source_path(mc.material_id);
        ImGui::TextUnformatted(src.empty() ? "Unknown" : src.stem().string().c_str());
    }
    ImGui::Separator();
    ImGui::Spacing();

    PBRMaterialParams params = mat->get_pbr_params();
    bool              dirty  = false;

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
    dirty |= ImGui::ColorEdit4("Albedo Colour", params.albedo_colour.data, ImGuiColorEditFlags_AlphaBar);
    dirty |= ImGui::ColorEdit4("Emissive Colour", params.emissive_colour.data, ImGuiColorEditFlags_AlphaBar);
    dirty |= ImGui::DragFloat("Alpha Cutoff", &params.alpha_cutoff, 0.01f, 0.0f, 1.0f);
    dirty |= ImGui::DragFloat("Emissive Intensity", &params.emissive_intensity, 0.1f, 0.0f, 100.0f);
    ImGui::PopItemWidth();

    ImGui::Spacing();

    if (ImGui::BeginTable("##tex_slots", 3, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("##thumb", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("##name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##clear", ImGuiTableColumnFlags_WidthFixed, 20.0f);

        GRITexture2D* white_tex = EditorResourceCache::get().get_white_texture();

        if (is_inline)
        {
            dirty |= draw_pbr_texture_slot("Albedo", params.albedo_tex, &mc.albedo_tex, white_tex);
            dirty |= draw_pbr_texture_slot("Normal", params.normal_tex, &mc.normal_tex);
            dirty |= draw_pbr_texture_slot("Roughness", params.roughness_tex, &mc.roughness_tex);
            dirty |= draw_pbr_texture_slot("Metallic", params.metallic_tex, &mc.metallic_tex);
            dirty |= draw_pbr_texture_slot("AO", params.ao_tex, &mc.ao_tex, white_tex);
            dirty |= draw_pbr_texture_slot("Emissive", params.emissive_tex, &mc.emissive_tex);
        }
        else
        {
            dirty |= draw_pbr_texture_slot("Albedo", params.albedo_tex, nullptr, white_tex);
            dirty |= draw_pbr_texture_slot("Normal", params.normal_tex);
            dirty |= draw_pbr_texture_slot("Roughness", params.roughness_tex);
            dirty |= draw_pbr_texture_slot("Metallic", params.metallic_tex);
            dirty |= draw_pbr_texture_slot("AO", params.ao_tex, nullptr, white_tex);
            dirty |= draw_pbr_texture_slot("Emissive", params.emissive_tex);
        }

        ImGui::EndTable();
    }

    if (dirty)
    {
        if (is_inline)
        {
            mc.albedo_colour      = params.albedo_colour;
            mc.emissive_colour    = params.emissive_colour;
            mc.alpha_cutoff       = params.alpha_cutoff;
            mc.emissive_intensity = params.emissive_intensity;
        }
        mat->update_pbr_params(params);
    }
}

} // namespace Ignis
