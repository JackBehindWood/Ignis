#ifdef ENGINE_IMGUI

#include "igpch.h"
#include "ImGuiLayer.h"

#include <imgui/imgui.h>

namespace Ignis
{

void ImGuiLayer::apply_dark_theme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4*     c     = style.Colors;

    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 2.0f;
    style.GrabRounding      = 2.0f;
    style.TabRounding       = 3.0f;
    style.PopupRounding     = 3.0f;
    style.ScrollbarRounding = 2.0f;
    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 0.0f;
    style.FrameBorderSize   = 0.0f;
    style.TabBorderSize     = 0.0f;
    style.TabBarBorderSize  = 1.0f;
    style.IndentSpacing     = 14.0f;
    style.WindowPadding     = {6.0f, 6.0f};
    style.FramePadding      = {5.0f, 3.0f};
    style.ItemSpacing       = {6.0f, 4.0f};
    style.ScrollbarSize     = 11.0f;
    style.GrabMinSize       = 8.0f;

    c[ImGuiCol_Text]                      = {0.87f, 0.87f, 0.87f, 1.00f};
    c[ImGuiCol_TextDisabled]              = {0.40f, 0.40f, 0.40f, 1.00f};
    c[ImGuiCol_WindowBg]                  = {0.114f, 0.114f, 0.114f, 1.00f};
    c[ImGuiCol_ChildBg]                   = {0.098f, 0.098f, 0.098f, 1.00f};
    c[ImGuiCol_PopupBg]                   = {0.135f, 0.135f, 0.135f, 1.00f};
    c[ImGuiCol_Border]                    = {0.180f, 0.180f, 0.180f, 1.00f};
    c[ImGuiCol_BorderShadow]              = {0.000f, 0.000f, 0.000f, 0.00f};
    c[ImGuiCol_FrameBg]                   = {0.157f, 0.157f, 0.157f, 1.00f};
    c[ImGuiCol_FrameBgHovered]            = {0.196f, 0.196f, 0.196f, 1.00f};
    c[ImGuiCol_FrameBgActive]             = {0.235f, 0.235f, 0.235f, 1.00f};
    c[ImGuiCol_TitleBg]                   = {0.078f, 0.078f, 0.078f, 1.00f};
    c[ImGuiCol_TitleBgActive]             = {0.094f, 0.094f, 0.094f, 1.00f};
    c[ImGuiCol_TitleBgCollapsed]          = {0.078f, 0.078f, 0.078f, 1.00f};
    c[ImGuiCol_MenuBarBg]                 = {0.090f, 0.090f, 0.090f, 1.00f};
    c[ImGuiCol_ScrollbarBg]               = {0.086f, 0.086f, 0.086f, 1.00f};
    c[ImGuiCol_ScrollbarGrab]             = {0.220f, 0.220f, 0.220f, 1.00f};
    c[ImGuiCol_ScrollbarGrabHovered]      = {0.275f, 0.275f, 0.275f, 1.00f};
    c[ImGuiCol_ScrollbarGrabActive]       = {0.350f, 0.350f, 0.350f, 1.00f};
    c[ImGuiCol_CheckMark]                 = {0.26f, 0.59f, 1.00f, 1.00f};
    c[ImGuiCol_SliderGrab]                = {0.26f, 0.59f, 1.00f, 0.90f};
    c[ImGuiCol_SliderGrabActive]          = {0.40f, 0.70f, 1.00f, 1.00f};
    c[ImGuiCol_Button]                    = {0.165f, 0.165f, 0.165f, 1.00f};
    c[ImGuiCol_ButtonHovered]             = {0.220f, 0.220f, 0.220f, 1.00f};
    c[ImGuiCol_ButtonActive]              = {0.290f, 0.290f, 0.290f, 1.00f};
    c[ImGuiCol_Header]                    = {0.18f, 0.37f, 0.65f, 0.50f};
    c[ImGuiCol_HeaderHovered]             = {0.22f, 0.44f, 0.75f, 0.75f};
    c[ImGuiCol_HeaderActive]              = {0.26f, 0.52f, 0.90f, 1.00f};
    c[ImGuiCol_Separator]                 = {0.180f, 0.180f, 0.180f, 1.00f};
    c[ImGuiCol_SeparatorHovered]          = {0.26f, 0.59f, 1.00f, 0.60f};
    c[ImGuiCol_SeparatorActive]           = {0.26f, 0.59f, 1.00f, 1.00f};
    c[ImGuiCol_ResizeGrip]                = {0.26f, 0.59f, 1.00f, 0.20f};
    c[ImGuiCol_ResizeGripHovered]         = {0.26f, 0.59f, 1.00f, 0.55f};
    c[ImGuiCol_ResizeGripActive]          = {0.26f, 0.59f, 1.00f, 0.90f};
    c[ImGuiCol_Tab]                       = {0.094f, 0.094f, 0.094f, 1.00f};
    c[ImGuiCol_TabHovered]                = {0.180f, 0.180f, 0.180f, 1.00f};
    c[ImGuiCol_TabSelected]               = {0.140f, 0.140f, 0.140f, 1.00f};
    c[ImGuiCol_TabSelectedOverline]       = {0.26f, 0.59f, 1.00f, 1.00f};
    c[ImGuiCol_TabDimmed]                 = {0.078f, 0.078f, 0.078f, 1.00f};
    c[ImGuiCol_TabDimmedSelected]         = {0.114f, 0.114f, 0.114f, 1.00f};
    c[ImGuiCol_TabDimmedSelectedOverline] = {0.26f, 0.59f, 1.00f, 0.25f};
    c[ImGuiCol_DockingPreview]            = {0.26f, 0.59f, 1.00f, 0.55f};
    c[ImGuiCol_DockingEmptyBg]            = {0.078f, 0.078f, 0.078f, 1.00f};
    c[ImGuiCol_PlotLines]                 = {0.26f, 0.59f, 1.00f, 1.00f};
    c[ImGuiCol_PlotLinesHovered]          = {0.40f, 0.70f, 1.00f, 1.00f};
    c[ImGuiCol_PlotHistogram]             = {0.26f, 0.59f, 1.00f, 1.00f};
    c[ImGuiCol_PlotHistogramHovered]      = {0.40f, 0.70f, 1.00f, 1.00f};
    c[ImGuiCol_NavHighlight]              = {0.26f, 0.59f, 1.00f, 1.00f};
}

ImGuiLayer::ImGuiLayer()
    : Layer("ImGuiLayer")
{
}

void ImGuiLayer::set_ini_path(const String& path)
{
    m_ini_path                 = path;
    ImGui::GetIO().IniFilename = m_ini_path.c_str();
}

void ImGuiLayer::register_drawable(IImGuiDrawable* d)
{
    s_drawables.push_back(d);
}

void ImGuiLayer::unregister_drawable(IImGuiDrawable* d)
{
    auto it = std::find(s_drawables.begin(), s_drawables.end(), d);
    if (it != s_drawables.end())
    {
        s_drawables.erase(it);
    }
}

void ImGuiLayer::event(Event& e)
{
    ImGuiIO& io = ImGui::GetIO();

    if (e.get_category_flags() & EventCategoryMouse)
    {
        if (io.WantCaptureMouse)
        {
            e.handled = true;
        }
    }
    else if (e.get_category_flags() & EventCategoryKeyboard)
    {
        if (io.WantCaptureKeyboard || ImGui::IsAnyItemActive())
        {
            e.handled = true;
        }
    }
}

} // namespace Ignis

#endif
