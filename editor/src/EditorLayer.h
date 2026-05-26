#pragma once

#include <Ignis.h>
#include <Ignis/Rendering/RenderGraph/RGBuilder.h>

namespace Ignis
{
class EditorLayer : public Layer
{
private:
    Scene         m_active_scene;
    SceneRenderer m_scene_renderer;
    RGBuilder     m_builder;
    AssetID       m_scene_texture_id{UUID::s_invalid};
    bool          m_scene_ready = false;

public:
    EditorLayer();
    virtual ~EditorLayer() = default;

    virtual void attach() override;
    virtual void detach() override;
    virtual void event(Event& event) override;

    bool key_pressed(KeyPressedEvent& e);

    void update(Timestep ts) override;
};
} // namespace Ignis
