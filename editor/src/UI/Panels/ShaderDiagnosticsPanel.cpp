#include "edpch.h"
#include "ShaderDiagnosticsPanel.h"

#include "UI/SceneEditor/SceneEditorContext.h"
#include <Ignis/Rendering/Renderer.h>
#include <Ignis/Rendering/Shaders/ShaderCache.h>
#include <Ignis/Scene/Components/Components.h>

namespace Ignis
{

PanelId ShaderDiagnosticsPanel::get_id() const
{
    return 7;
}

void ShaderDiagnosticsPanel::draw(IWorkspaceData* ctx)
{
    auto& sc = ShaderCache::get();

    // --- Cache statistics ---
    auto stats = sc.snapshot_stats();
    ImGui::Text("Cached Shaders: %zu", stats.size());

    if (ImGui::BeginTable("##shader_cache", 3,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
                          ImVec2(0.0f, 200.0f)))
    {
        ImGui::TableSetupColumn("Source File", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Variant Hash", ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableSetupColumn("Bytecode Hash", ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableHeadersRow();

        for (const auto& s : stats)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(s.source_path.empty() ? "(inline)" : s.source_path.filename().string().c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%016llx", static_cast<unsigned long long>(s.variant_hash));
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%016llx", static_cast<unsigned long long>(s.bytecode_hash));
        }
        ImGui::EndTable();
    }

    if (ImGui::Button("Evict All & Force Recompile"))
    {
        uint32_t count = 0;
        for (const auto& s : stats)
        {
            if (!s.source_path.empty())
            {
                sc.remove(s.source_path);
                ++count;
            }
        }
        m_evict_status = "Evicted " + std::to_string(count) + " shader(s)";
    }
    if (!m_evict_status.empty())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", m_evict_status.c_str());
    }

    ImGui::Separator();

    // --- Pipeline layout for selected entity's material ---
    auto* data = ctx ? static_cast<SceneEditorData*>(ctx) : nullptr;
    if (!data || !data->selected_entity || !data->selected_entity->has_component<MaterialComponent>())
    {
        ImGui::TextDisabled("Select an entity with a MaterialComponent to inspect its pipeline layout.");
        return;
    }

    const MaterialComponent& mc = data->selected_entity->get_component<MaterialComponent>();
    SharedPtr<Material>      mat =
        mc.material_id ? Renderer::get_resource_cache().find_material(static_cast<uint64_t>(mc.material_id)) : nullptr;

    if (!mat)
    {
        ImGui::TextDisabled("Material not yet loaded.");
        return;
    }

    ImGui::Text("Pipeline Layout — %llu", static_cast<uint64_t>(mc.material_id));

    const MergedPipelineReflection& layout = mat->get_pipeline_layout();

    if (ImGui::BeginTable("##pipeline_layout", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Set", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Binding", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Stages", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();

        for (const auto& b : layout.bindings)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(b.name.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", b.set);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%u", b.binding);
            ImGui::TableSetColumnIndex(3);
            switch (b.type)
            {
                case MergedResourceType::UniformBuffer:
                    ImGui::TextUnformatted("UBO");
                    break;
                case MergedResourceType::StorageBuffer:
                    ImGui::TextUnformatted("SSBO");
                    break;
                case MergedResourceType::StorageTexture:
                    ImGui::TextUnformatted("Image");
                    break;
                case MergedResourceType::SeparateImage:
                    ImGui::TextUnformatted("Texture");
                    break;
                case MergedResourceType::SeparateSampler:
                    ImGui::TextUnformatted("Sampler");
                    break;
                default:
                    ImGui::TextUnformatted("?");
                    break;
            }
            ImGui::TableSetColumnIndex(4);
            char stages[4] = {};
            int  si        = 0;
            if (b.stage_mask & 0x01)
            {
                stages[si++] = 'V';
            }
            if (b.stage_mask & 0x02)
            {
                stages[si++] = 'P';
            }
            ImGui::TextUnformatted(stages);
        }
        ImGui::EndTable();
    }
}

} // namespace Ignis
