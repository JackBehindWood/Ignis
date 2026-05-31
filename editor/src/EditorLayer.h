#pragma once

#include <Ignis.h>
#include <Ignis/Rendering/RenderGraph/RGBuilder.h>
#include <Ignis/Rendering/RenderGraph/RGResource.h>

#ifdef ENGINE_IMGUI
#include "UI/Panels/PanelRegistry.h"
#include "UI/Workspace/WorkspaceManager.h"
#endif

namespace Ignis
{
class EditorLayer : public Layer
{
private:
    Scene         m_active_scene;
    SceneRenderer m_scene_renderer;
    RGBuilder     m_builder;
    bool          m_scene_ready = false;

#ifdef ENGINE_IMGUI
    PanelRegistry    m_panel_registry;
    WorkspaceManager m_workspace_manager{m_panel_registry};
#endif

    void draw_grid(RGTextureHandle color, RGTextureHandle depth);
    void draw_outline_composite(RGTextureHandle color_rt, RGTextureHandle mask_rt);

public:
    EditorLayer();
    virtual ~EditorLayer() = default;

    virtual void attach() override;
    virtual void detach() override;
    virtual void update(Timestep ts) override;
    virtual void render() override;
    virtual void event(Event& event) override;

    bool key_pressed(KeyPressedEvent& e);
    bool window_resized(WindowResizeEvent& e);
};
} // namespace Ignis
