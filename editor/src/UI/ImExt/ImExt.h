#pragma once

#include <Ignis/Math/Mat4.h>
#include <Ignis/Math/Transform.h>
#include "UI/SceneEditor/SceneEditorContext.h"

namespace ImExt
{

using namespace Ignis;

struct GizmoCtx
{
    ImVec2      vp_min;  // screen-space top-left of the rendered scene image
    ImVec2      vp_size; // viewport pixel dimensions
    Math::Mat4f view_proj;
    Math::Mat4f view;
    Math::Mat4f proj;
    Math::Vec3f cam_pos;
};

struct WorldRay
{
    Math::Vec3f origin;
    Math::Vec3f dir;
};

// Returns true when the transform was mutated this frame.
bool ManipulateTransform(Math::Transformf& transform, GizmoMode mode, const GizmoCtx& ctx);

// Draws a small XYZ orientation widget at `center`. Returns clicked axis index
// (0=+X 1=-X 2=+Y 3=-Y 4=+Z 5=-Z) or -1 if nothing was clicked.
int DrawCameraOrientation(const Math::Mat4f& view, ImVec2 center, float size = 50.0f);

// Constructs a world-space ray from a screen-space pixel, viewport bounds, and camera matrices.
WorldRay screen_to_world_ray(ImVec2 screen_px, ImVec2 vp_min, ImVec2 vp_size, const CameraData& cam);

// True when a gizmo handle is actively being dragged this frame.
bool IsGizmoActive();

namespace DragDrop
{

template <typename T>
struct Source
{
    explicit Source(ImGuiDragDropFlags flags = 0)
        : m_active(ImGui::BeginDragDropSource(flags))
    {
    }
    ~Source()
    {
        if (m_active)
        {
            ImGui::EndDragDropSource();
        }
    }
    Source(const Source&)             = delete;
    Source&  operator=(const Source&) = delete;
    explicit operator bool() const
    {
        return m_active;
    }
    void set_payload(const T& data) const
    {
        ImGui::SetDragDropPayload(T::k_type, &data, sizeof(T));
    }
    void set_tooltip(const char* fmt, ...) const
    {
        va_list args;
        va_start(args, fmt);
        ImGui::SetTooltipV(fmt, args);
        va_end(args);
    }

private:
    bool m_active;
};

template <typename T>
struct Target
{
    Target()
        : m_active(false),
          m_typed(false)
    {
        m_active = ImGui::BeginDragDropTarget();
        if (m_active)
        {
            const ImGuiPayload* raw = ImGui::GetDragDropPayload();
            m_typed                 = raw && raw->IsDataType(T::k_type);
        }
    }
    ~Target()
    {
        if (m_active)
        {
            ImGui::EndDragDropTarget();
        }
    }
    Target(const Target&)             = delete;
    Target&  operator=(const Target&) = delete;
    explicit operator bool() const
    {
        return m_typed;
    }
    bool is_copy() const
    {
        return ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
    }
    const T* peek() const
    {
        if (!m_typed)
        {
            return nullptr;
        }
        const ImGuiPayload* p = ImGui::AcceptDragDropPayload(T::k_type, ImGuiDragDropFlags_AcceptBeforeDelivery |
                                                                            ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
        return p ? static_cast<const T*>(p->Data) : nullptr;
    }
    const T* accept() const
    {
        if (!m_typed)
        {
            return nullptr;
        }
        const ImGuiPayload* p = ImGui::AcceptDragDropPayload(T::k_type);
        return p ? static_cast<const T*>(p->Data) : nullptr;
    }
    void draw_compat_feedback(bool compat, ImVec2 rmin, ImVec2 rmax, float rounding = 2.f) const
    {
        ImU32 col = compat ? IM_COL32(100, 200, 100, 220) : IM_COL32(200, 80, 80, 220);
        ImGui::GetWindowDrawList()->AddRect(rmin, rmax, col, rounding, 0, 1.5f);
    }
    void draw_operation_feedback(ImVec2 rmin, ImVec2 rmax, float rounding = 4.f) const
    {
        ImU32 col = is_copy() ? IM_COL32(100, 200, 100, 200) : IM_COL32(100, 150, 255, 200);
        ImGui::GetWindowDrawList()->AddRect(rmin, rmax, col, rounding, 0, 2.f);
    }

private:
    bool m_active;
    bool m_typed;
};

template <typename T>
struct WindowTarget
{
    explicit WindowTarget(const char* dest_name)
        : m_active(false),
          m_typed(false),
          m_dest_name(dest_name)
    {
        ImGuiWindow* w = ImGui::GetCurrentWindow();
        m_active       = ImGui::BeginDragDropTargetCustom(w->Rect(), w->ID);
        if (m_active)
        {
            const ImGuiPayload* raw = ImGui::GetDragDropPayload();
            m_typed                 = raw && raw->IsDataType(T::k_type);
            if (m_typed)
            {
                bool  copy = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
                ImU32 tint = copy ? IM_COL32(100, 200, 100, 30) : IM_COL32(100, 150, 255, 30);
                w->DrawList->AddRectFilled(w->Rect().Min, w->Rect().Max, tint);
                ImGui::SetTooltip("%s into %s", copy ? "Copy" : "Move", m_dest_name);
            }
        }
    }
    ~WindowTarget()
    {
        if (m_active)
        {
            ImGui::EndDragDropTarget();
        }
    }
    WindowTarget(const WindowTarget&)            = delete;
    WindowTarget& operator=(const WindowTarget&) = delete;
    explicit      operator bool() const
    {
        return m_typed;
    }
    bool is_copy() const
    {
        return ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
    }
    const T* peek() const
    {
        if (!m_typed)
        {
            return nullptr;
        }
        const ImGuiPayload* p = ImGui::AcceptDragDropPayload(T::k_type, ImGuiDragDropFlags_AcceptBeforeDelivery |
                                                                            ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
        return p ? static_cast<const T*>(p->Data) : nullptr;
    }
    const T* accept() const
    {
        if (!m_typed)
        {
            return nullptr;
        }
        const ImGuiPayload* p = ImGui::AcceptDragDropPayload(T::k_type);
        return p ? static_cast<const T*>(p->Data) : nullptr;
    }

private:
    bool        m_active;
    bool        m_typed;
    const char* m_dest_name;
};

} // namespace DragDrop

} // namespace ImExt
