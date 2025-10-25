#pragma once

#include "Ignis/Core/Timestep.h"

namespace Ignis 
{

	class Layer
	{
	public:
		Layer(const String& name = "Layer") : m_debug_name(name) {}
		virtual ~Layer() = default;

		virtual void attach() {}
		virtual void detach() {}
		virtual void update(Timestep ts) {}

		const String& get_name() const { return m_debug_name; }
	protected:
		String m_debug_name;
	};

}