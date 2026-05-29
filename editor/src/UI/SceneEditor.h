#pragma once

#include <Ignis/UI/IImGuiDrawable.h>
#include <imgui.h>
#include <Ignis/Scene/Scene.h>
#include <Ignis/Scene/Entity.h>
#include <Ignis/Scene/SceneRenderer.h>

#include "SceneTreePanel.h"
#include "ConsolePanel.h"
#include "ViewportPanel.h"
#include "PropertyPanel.h"

namespace Ignis
{

enum class SimulationState : uint8_t
{
    Stopped,
    Playing,
    Paused,
};

enum class GizmoMode : uint8_t
{
    Translate,
    Rotate,
    Scale,
};

class SceneEditor : public IImGuiDrawable
{
public:
    SceneEditor() = default;

    void set_scene(Scene* scene)
    {
        m_scene = scene;
    }
    void set_scene_renderer(SceneRenderer* sr)
    {
        m_scene_renderer = sr;
    }

    void draw_imgui() override;

    bool has_pending_resize() override
    {
        return m_viewport.has_pending_resize();
    }
    void flush_resize() override
    {
        m_viewport.flush_resize(*m_scene_renderer);
    }

    ConsolePanel& get_console()
    {
        return m_console;
    }

private:
    void setup_dockspace_host_window();
    void build_default_layout(ImGuiID dockspace_id);
    void draw_menu_bar();
    void draw_toolbar();

    Scene*         m_scene          = nullptr;
    SceneRenderer* m_scene_renderer = nullptr;
    Entity         m_selected_entity;

    bool m_first_run       = true;
    bool m_show_scene_tree = true;
    bool m_show_viewport   = true;
    bool m_show_console    = true;
    bool m_show_properties = true;

    SimulationState m_sim_state = SimulationState::Stopped;
    GizmoMode       m_gizmo     = GizmoMode::Translate;

    SceneTreePanel m_scene_tree;
    ConsolePanel   m_console;
    ViewportPanel  m_viewport;
    PropertyPanel  m_properties;
};

} // namespace Ignis
