#include "edpch.h"
#include "IBLStudioPanel.h"

#include "Asset/EditorAssetManager.h"
#include <Ignis/Rendering/Renderer.h>
#include <Ignis/Rendering/GlobalEngineCache.h>
#include <Ignis/Rendering/RenderTexture2D.h>
#include <Ignis/Rendering/GRI/GRIDefinitions.h>
#include <Ignis/Core/FileDialog.h>

namespace Ignis
{

PanelId IBLStudioPanel::get_id() const
{
    return 6;
}

void IBLStudioPanel::update(float /*ts*/, IWorkspaceData* /*ctx*/)
{
    if (!m_pending_bake)
    {
        return;
    }
    m_pending_bake = false;

    const Path hdr_path(m_hdr_path);
    if (!Filesystem::exists(hdr_path))
    {
        m_status = "Error: file not found";
        m_state  = BakeState::Failed;
        return;
    }

    m_status = "Baking Environment...";
    m_state  = BakeState::Baking;

    IBLBakeResult result = EditorAssetManager::get().cook_ibl_environment(hdr_path);

    if (!result.irradiance_cube || !result.prefilter_cube || !result.brdf_lut)
    {
        m_status = "Bake Failed";
        m_state  = BakeState::Failed;
        return;
    }

    GlobalEngineCache& cache = Renderer::get_global_cache();

    auto wrap_cube = [](GRITexture2DPtr tex, uint32_t sz, GRIPixelFormat fmt)
    { return create_shared<RenderTexture2D>(std::move(tex), sz, sz * 6u, fmt); };

    cache.set_irradiance_cube(wrap_cube(std::move(result.irradiance_cube), 32u, GRIPixelFormat::RGBA16Float));
    cache.set_prefilter_cube(wrap_cube(std::move(result.prefilter_cube), 256u, GRIPixelFormat::RGBA16Float));
    cache.set_brdf_lut(
        create_shared<RenderTexture2D>(std::move(result.brdf_lut), 512u, 512u, GRIPixelFormat::RG16Float));

    m_status = "Bake Complete";
    m_state  = BakeState::Done;
}

void IBLStudioPanel::draw(IWorkspaceData* /*ctx*/)
{
    ImGui::InputText("HDR Path", m_hdr_path, sizeof(m_hdr_path));
    ImGui::SameLine();
    if (ImGui::Button("Browse..."))
    {
        FileDialogOptions opts;
        opts.filters = Vector<String>{"hdr", "exr"};
        if (auto path = FileDialog::open(FileDialogMode::ImportAsset, opts))
        {
            const String s = path->string();
            std::snprintf(m_hdr_path, sizeof(m_hdr_path), "%s", s.c_str());
        }
    }

    const bool baking = (m_state == BakeState::Baking);
    if (baking)
    {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Bake Environment") && !baking)
    {
        m_pending_bake = true;
    }
    if (baking)
    {
        ImGui::EndDisabled();
    }

    ImVec4 status_col = {0.6f, 0.6f, 0.6f, 1.0f};
    if (m_state == BakeState::Baking)
    {
        status_col = {1.0f, 0.85f, 0.0f, 1.0f};
    }
    if (m_state == BakeState::Done)
    {
        status_col = {0.3f, 1.0f, 0.3f, 1.0f};
    }
    if (m_state == BakeState::Failed)
    {
        status_col = {1.0f, 0.3f, 0.3f, 1.0f};
    }

    ImGui::TextColored(status_col, "%s", m_status.c_str());

    ImGui::Separator();
    ImGui::TextDisabled("Bakes equirectangular HDR to:");
    ImGui::TextDisabled("  env_cube 1024 * irradiance_cube 32 * prefilter_cube 256 (5 mips) * brdf_lut 512");
}

} // namespace Ignis
