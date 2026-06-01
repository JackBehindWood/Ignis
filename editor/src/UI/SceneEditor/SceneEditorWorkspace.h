#pragma once

#include "../Workspace/IWorkspace.h"
#include "../Panels/PanelRegistry.h"
#include "SceneEditorContext.h"

namespace Ignis
{

class SceneEditorWorkspace : public WorkspaceBase
{
public:
    SceneEditorWorkspace(Scene& scene, SceneRenderer& sr, PanelRegistry& registry, CommandDispatcher& dispatcher);
    ~SceneEditorWorkspace();

    const WorkspaceDefinition& definition() const override
    {
        return m_def;
    }
    IWorkspaceData* data() override
    {
        return &m_data;
    }
    void draw_menu_bar() override;

    bool has_pending_resize() const override;
    void flush_resize() override;

private:
    static WorkspaceDefinition make_definition(PanelRegistry& registry);

    WorkspaceDefinition m_def;
    SceneEditorData     m_data;
    Entity              m_selected_entity;
    SimulationState     m_sim_state = SimulationState::Stopped;
    GizmoMode           m_gizmo     = GizmoMode::Translate;
    InputContext        m_viewport_ctx{"EditorViewport", 200};
};

} // namespace Ignis
