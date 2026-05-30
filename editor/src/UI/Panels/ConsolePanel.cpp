#include "edpch.h"
#include "ConsolePanel.h"

#include <imgui.h>

namespace Ignis
{

static constexpr PanelId k_id = 3;

PanelId ConsolePanel::get_id() const
{
    return k_id;
}

static const char* level_label(spdlog::level::level_enum lvl)
{
    switch (lvl)
    {
        case spdlog::level::trace:
            return "[TRACE]";
        case spdlog::level::debug:
            return "[DEBUG]";
        case spdlog::level::info:
            return "[INFO ]";
        case spdlog::level::warn:
            return "[WARN ]";
        case spdlog::level::err:
            return "[ERROR]";
        case spdlog::level::critical:
            return "[CRIT ]";
        default:
            return "[?????]";
    }
}

static ImVec4 level_color(spdlog::level::level_enum lvl)
{
    switch (lvl)
    {
        case spdlog::level::trace:
            return {0.55f, 0.55f, 0.55f, 1.0f};
        case spdlog::level::debug:
            return {0.50f, 0.80f, 0.50f, 1.0f};
        case spdlog::level::info:
            return {0.85f, 0.85f, 0.85f, 1.0f};
        case spdlog::level::warn:
            return {1.00f, 0.85f, 0.20f, 1.0f};
        case spdlog::level::err:
            return {1.00f, 0.40f, 0.40f, 1.0f};
        case spdlog::level::critical:
            return {1.00f, 0.20f, 0.20f, 1.0f};
        default:
            return {1.00f, 1.00f, 1.00f, 1.0f};
    }
}

void ConsolePanel::draw(IWorkspaceData*)
{
    ConsoleSink& sink = ConsoleSink::get();

    if (ImGui::Button("Clear"))
    {
        sink.clear();
    }
    ImGui::SameLine();

    const char* level_names[] = {"Trace", "Debug", "Info", "Warn", "Error", "Critical"};
    for (int i = 0; i < 6; ++i)
    {
        ImGui::SameLine();
        ImGui::Checkbox(level_names[i], &m_filter[i]);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_auto_scroll);

    ImGui::Separator();
    ImGui::BeginChild("##console_scroll", {0, 0}, false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const LogEntry& e : sink.entries())
    {
        int idx = static_cast<int>(e.level);
        if (idx < 0 || idx > 5 || !m_filter[idx])
        {
            continue;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, level_color(e.level));
        ImGui::TextUnformatted(level_label(e.level));
        ImGui::SameLine();
        ImGui::TextUnformatted(e.message.c_str());
        ImGui::PopStyleColor();
    }

    if (m_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}

} // namespace Ignis
