#include "edpch.h"
#include "ViewportPanel.h"

#include <imgui.h>

namespace Ignis
{

void ViewportPanel::draw(SceneRenderer& scene_renderer)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");

    ImVec2   avail = ImGui::GetContentRegionAvail();
    uint32_t aw    = static_cast<uint32_t>(avail.x);
    uint32_t ah    = static_cast<uint32_t>(avail.y);

    if (aw > 0 && ah > 0 && (aw != m_last_w || ah != m_last_h))
    {
        m_pending_w      = aw;
        m_pending_h      = ah;
        m_resize_pending = true;
    }

    GRITexture2D* color_rt = scene_renderer.get_color_rt();
    if (color_rt)
    {
        ImTextureID tex_id = reinterpret_cast<ImTextureID>(color_rt->get_native_handle());
        ImGui::Image(tex_id,
                     {static_cast<float>(m_last_w ? m_last_w : aw), static_cast<float>(m_last_h ? m_last_h : ah)});
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::flush_resize(SceneRenderer& scene_renderer)
{
    if (!m_resize_pending)
    {
        return;
    }

    scene_renderer.resize(m_pending_w, m_pending_h);
    m_last_w         = m_pending_w;
    m_last_h         = m_pending_h;
    m_resize_pending = false;
}

} // namespace Ignis
