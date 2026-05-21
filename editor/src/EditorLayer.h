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
        SharedPtr<AssetMaterial>  m_material;
        SharedPtr<RenderMesh>     m_mesh;
        GRIBufferPtr              m_uniform_buffer;
        SharedPtr<AssetTexture2D> m_texture;
    };
}