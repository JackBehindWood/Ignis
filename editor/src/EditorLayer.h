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

		void update(Timestep ts) override;
    };
}