#include "edpch.h"
#include "ConsolePanel.h"

#include <Ignis/UI/ImGuiLayer.h>

namespace Ignis
{

static constexpr PanelId k_id = 3;

PanelId ConsolePanel::get_id() const
{
    return k_id;
}

static const char* level_label(spdlog::level::level_enum lvl)
{
    if (static_cast<int>(lvl) == 10)
    {
        return "[USER]";
    }

    switch (lvl)
    {
        case spdlog::level::trace:
            return "[TRACE]";
        case spdlog::level::debug:
            return "[DEBUG]";
        case spdlog::level::info:
            return "[INFO]";
        case spdlog::level::warn:
            return "[WARN]";
        case spdlog::level::err:
            return "[ERROR]";
        case spdlog::level::critical:
            return "[CRIT]";
        default:
            return "[?????]";
    }
}

static ImVec4 level_color(spdlog::level::level_enum lvl)
{
    if (static_cast<int>(lvl) == 10)
    {
        return {0.20f, 0.85f, 0.50f, 1.0f};
    }

    switch (lvl)
    {
        case spdlog::level::trace:
            return {0.40f, 0.40f, 0.45f, 1.0f};
        case spdlog::level::debug:
            return {0.55f, 0.45f, 0.75f, 1.0f};
        case spdlog::level::info:
            return {0.80f, 0.80f, 0.80f, 1.0f};
        case spdlog::level::warn:
            return {0.95f, 0.76f, 0.20f, 1.0f};
        case spdlog::level::err:
            return {0.90f, 0.35f, 0.30f, 1.0f};
        case spdlog::level::critical:
            return {0.95f, 0.18f, 0.12f, 1.0f};
        default:
            return {0.80f, 0.80f, 0.80f, 1.0f};
    }
}

void ConsolePanel::draw(IWorkspaceData*)
{
    ConsoleSink& sink = ConsoleSink::get();
    ImGuiIO&     io   = ImGui::GetIO();

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

    constexpr float k_input_h = 26.0f;
    const float scroll_h = ImGui::GetContentRegionAvail().y - k_input_h - ImGui::GetStyle().ItemSpacing.y * 2.0f - 1.0f;

    if (ImFont* mono = ImGuiLayer::mono_font())
    {
        ImGui::PushFont(mono);
    }
    ImGui::BeginChild("##console_scroll", {0.0f, scroll_h}, false, ImGuiWindowFlags_HorizontalScrollbar);

    static constexpr ImVec4 k_ts_color = {0.35f, 0.35f, 0.38f, 1.0f};

    // TODO: find a better way than to just loop each frame!
    for (const LogEntry& e : sink.entries())
    {
        int32_t idx = static_cast<int32_t>(e.level);
        if (idx != 10 && (idx < 0 || idx > 5 || !m_filter[idx]))
        {
            continue;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, k_ts_color);
        ImGui::TextUnformatted(e.timestamp.c_str());
        ImGui::PopStyleColor();

        ImGui::SameLine(0.0f, 6.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, level_color(e.level));
        ImGui::TextUnformatted(level_label(e.level));

        ImGui::SameLine(0.0f, 6.0f);
        ImGui::TextUnformatted(e.message.c_str());
        ImGui::PopStyleColor();
    }

    if (m_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();

    if (ImGuiLayer::mono_font())
    {
        ImGui::PopFont();
    }

    ImGui::Separator();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputTextWithHint("##cmd", "Run command...", m_input_buf, sizeof(m_input_buf),
                                 ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (m_input_buf[0] != '\0')
        {
            String input_str(m_input_buf);

            if (input_str[0] == '/')
            {
                // Execute command routine...
                IG_ERROR("Command system not in place yet!");
            }
            else
            {
                sink.push_user_input(input_str);
            }

            m_input_buf[0] = '\0';
            ImGui::SetKeyboardFocusHere(-1);
        }
    }
}

} // namespace Ignis
