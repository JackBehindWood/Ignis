#pragma once

#include <Ignis.h>

#ifdef ENGINE_IMGUI
#include "UI/Panels/PanelRegistry.h"
#include "UI/Commands/CommandDispatcher.h"
#include "UI/Workspace/WorkspaceManager.h"
#endif

namespace Ignis
{
class EditorLayer : public Layer
{
private:
    Scene         m_active_scene;
    SceneRenderer m_scene_renderer;
    bool          m_scene_ready = false;

#ifdef ENGINE_IMGUI
    PanelRegistry     m_panel_registry;
    CommandDispatcher m_dispatcher;
    WorkspaceManager  m_workspace_manager{m_panel_registry, m_dispatcher};
    InputContext      m_global_editor_ctx{"EditorGlobal", 100};
#endif

public:
    EditorLayer();
    virtual ~EditorLayer() = default;

    virtual void attach() override;
    virtual void detach() override;
    virtual void update(Timestep ts) override;
    virtual void render() override;
    virtual void event(Event& event) override;

    bool window_resized(WindowResizeEvent& e);
};
} // namespace Ignis
