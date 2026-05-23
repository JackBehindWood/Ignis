#pragma once

#include <Ignis.h>

namespace Ignis
{
    class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer() = default;

		virtual void attach() override;
		virtual void detach() override;
		virtual void event(Event& event) override;

		bool key_pressed(KeyPressedEvent& e);

		void update(Timestep ts) override;

    private:
        AssetID m_mesh_id;
        AssetID m_material_id;
        uint32_t m_index_count = 0;

        SharedPtr<AssetTexture2D> m_texture;
        GRIRenderPassInfo m_forward_pass;

        struct SceneUniforms { float transform[16]; };
        SceneUniforms m_transform{};
    };
}
