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
        SharedPtr<Shader>   m_shader;
        GRIPipelineStatePtr m_pipeline_state;
        GRIBufferPtr        m_vertex_buffer;
		GRIBufferPtr		m_index_buffer;
        GRIBufferPtr        m_uniform_buffer;
    };
}