#pragma once

#include "Ignis/Core/Layer.h"

namespace Ignis 
{

	class LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		void push_layer(Layer* layer);
		void push_overlay(Layer* overlay);
		void pop_layer(Layer* layer);
		void pop_overlay(Layer* overlay);

		Vector<Layer*>::iterator begin() { return m_layers.begin(); }
		Vector<Layer*>::iterator end() { return m_layers.end(); }
		Vector<Layer*>::reverse_iterator rbegin() { return m_layers.rbegin(); }
		Vector<Layer*>::reverse_iterator rend() { return m_layers.rend(); }

		Vector<Layer*>::const_iterator begin() const { return m_layers.begin(); }
		Vector<Layer*>::const_iterator end()	const { return m_layers.end(); }
		Vector<Layer*>::const_reverse_iterator rbegin() const { return m_layers.rbegin(); }
		Vector<Layer*>::const_reverse_iterator rend() const { return m_layers.rend(); }
	private:
		Vector<Layer*> m_layers;
		uint32_t m_layers_insert_index = 0;
	};

}