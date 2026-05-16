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

		void update(Timestep ts) override;

    private:
        GRIVertexShaderPtr  m_vertex_shader;
        GRIPixelShaderPtr   m_pixel_shader;
        GRIPipelineStatePtr m_pipeline_state;
        GRIBufferPtr        m_vertex_buffer;
		GRIBufferPtr		m_index_buffer;
        GRIBufferPtr        m_uniform_buffer;
    };
}