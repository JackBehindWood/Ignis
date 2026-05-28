#pragma once

#include <Ignis.h>
#include <Ignis/Rendering/RenderGraph/RGBuilder.h>
#include <Ignis/Rendering/RenderGraph/RGResource.h>
#include <Ignis/Rendering/Shaders/RenderShader.h>
#include <Ignis/Rendering/Material.h>
#include "Ignis/Scene/FlyCamera.h"

namespace Ignis
{
class EditorLayer : public Layer
{
private:
    Scene         m_active_scene;
    SceneRenderer m_scene_renderer;
    FlyCamera     m_fly_camera;
    RGBuilder     m_builder;
    AssetID       m_scene_texture_id{UUID::s_invalid};
    bool          m_scene_ready = false;

    SharedPtr<RenderShader> m_grid_vs;
    SharedPtr<RenderShader> m_grid_ps;
    SharedPtr<Material>     m_grid_material;

    void draw_grid(RGTextureHandle bb);

public:
    EditorLayer();
    virtual ~EditorLayer() = default;

    virtual void attach() override;
    virtual void detach() override;
    virtual void event(Event& event) override;

    bool key_pressed(KeyPressedEvent& e);
    bool mouse_button_pressed(MouseButtonPressedEvent& e);
    bool mouse_button_released(MouseButtonReleasedEvent& e);
    bool mouse_moved(MouseMovedEvent& e);
    bool mouse_scrolled(MouseScrolledEvent& e);
    bool window_resized(WindowResizeEvent& e);

    void update(Timestep ts) override;
};
} // namespace Ignis
