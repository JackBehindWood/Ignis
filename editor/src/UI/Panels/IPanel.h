#pragma once

#include "IWorkspaceData.h"

namespace Ignis
{

using PanelId = uint32_t;

class IPanel
{
public:
    virtual ~IPanel() = default;

    virtual PanelId get_id() const = 0;

    // Must match the exact string passed to ImGui::Begin().
    // Use the ### ID splitter for panels with dynamic display text:
    // e.g. "Asset Browser (3)###AssetBrowser"
    virtual const char* get_title() const = 0;

    // Called once per frame before draw(), driven by WorkspaceBase::update().
    // ctx may be null for context-free panels.
    virtual void update(float ts, IWorkspaceData* ctx)
    {
    }

    // ctx is null for context-free panels (ConsolePanel, ProfilerPanel, etc.).
    // Context-needing panels static_cast to their concrete IWorkspaceData subtype.
    virtual void draw(IWorkspaceData* ctx) = 0;

    virtual bool has_pending_resize() const
    {
        return false;
    }
    virtual void flush_resize(IWorkspaceData* ctx)
    {
    }

    // Called by WorkspaceManager immediately before/after ImGui::Begin() for
    // this panel's window. Override to push per-window style vars.
    virtual void push_window_style()
    {
    }
    virtual void pop_window_style()
    {
    }

    virtual ImGuiWindowFlags get_window_flags() const
    {
        return ImGuiWindowFlags_NoCollapse;
    }

    virtual bool is_closeable() const
    {
        return false;
    }
};

} // namespace Ignis
