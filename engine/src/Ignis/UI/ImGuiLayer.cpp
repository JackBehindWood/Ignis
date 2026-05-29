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

    style.WindowRounding    = 4.0f;
    style.FrameRounding     = 3.0f;
    style.GrabRounding      = 3.0f;
    style.TabRounding       = 4.0f;
    style.ChildRounding     = 3.0f;
    style.PopupRounding     = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.IndentSpacing     = 16.0f;
    style.WindowPadding     = {8, 8};
    style.FramePadding      = {4, 3};
    style.ItemSpacing       = {6, 4};

    c[ImGuiCol_WindowBg]             = {0.10f, 0.10f, 0.10f, 1.00f};
    c[ImGuiCol_ChildBg]              = {0.12f, 0.12f, 0.12f, 1.00f};
    c[ImGuiCol_PopupBg]              = {0.13f, 0.13f, 0.13f, 1.00f};
    c[ImGuiCol_Border]               = {0.25f, 0.25f, 0.25f, 1.00f};
    c[ImGuiCol_FrameBg]              = {0.18f, 0.18f, 0.18f, 1.00f};
    c[ImGuiCol_FrameBgHovered]       = {0.22f, 0.22f, 0.22f, 1.00f};
    c[ImGuiCol_FrameBgActive]        = {0.27f, 0.27f, 0.27f, 1.00f};
    c[ImGuiCol_TitleBg]              = {0.08f, 0.08f, 0.08f, 1.00f};
    c[ImGuiCol_TitleBgActive]        = {0.10f, 0.10f, 0.10f, 1.00f};
    c[ImGuiCol_MenuBarBg]            = {0.10f, 0.10f, 0.10f, 1.00f};
    c[ImGuiCol_ScrollbarBg]          = {0.10f, 0.10f, 0.10f, 1.00f};
    c[ImGuiCol_ScrollbarGrab]        = {0.28f, 0.28f, 0.28f, 1.00f};
    c[ImGuiCol_ScrollbarGrabHovered] = {0.35f, 0.35f, 0.35f, 1.00f};
    c[ImGuiCol_ScrollbarGrabActive]  = {0.45f, 0.45f, 0.45f, 1.00f};
    c[ImGuiCol_CheckMark]            = {0.48f, 0.71f, 1.00f, 1.00f};
    c[ImGuiCol_SliderGrab]           = {0.48f, 0.71f, 1.00f, 1.00f};
    c[ImGuiCol_SliderGrabActive]     = {0.60f, 0.80f, 1.00f, 1.00f};
    c[ImGuiCol_Button]               = {0.20f, 0.20f, 0.20f, 1.00f};
    c[ImGuiCol_ButtonHovered]        = {0.30f, 0.30f, 0.30f, 1.00f};
    c[ImGuiCol_ButtonActive]         = {0.40f, 0.40f, 0.40f, 1.00f};
    c[ImGuiCol_Header]               = {0.20f, 0.40f, 0.70f, 0.55f};
    c[ImGuiCol_HeaderHovered]        = {0.25f, 0.50f, 0.85f, 0.80f};
    c[ImGuiCol_HeaderActive]         = {0.30f, 0.55f, 0.95f, 1.00f};
    c[ImGuiCol_Tab]                  = {0.13f, 0.13f, 0.13f, 1.00f};
    c[ImGuiCol_TabHovered]           = {0.25f, 0.50f, 0.85f, 0.80f};
    c[ImGuiCol_TabActive]            = {0.20f, 0.40f, 0.70f, 1.00f};
    c[ImGuiCol_TabUnfocused]         = {0.10f, 0.10f, 0.10f, 1.00f};
    c[ImGuiCol_TabUnfocusedActive]   = {0.15f, 0.30f, 0.55f, 1.00f};
    c[ImGuiCol_DockingPreview]       = {0.30f, 0.55f, 0.95f, 0.70f};
    c[ImGuiCol_DockingEmptyBg]       = {0.08f, 0.08f, 0.08f, 1.00f};
    c[ImGuiCol_Separator]            = {0.25f, 0.25f, 0.25f, 1.00f};
    c[ImGuiCol_Text]                 = {0.90f, 0.90f, 0.90f, 1.00f};
    c[ImGuiCol_TextDisabled]         = {0.45f, 0.45f, 0.45f, 1.00f};
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
