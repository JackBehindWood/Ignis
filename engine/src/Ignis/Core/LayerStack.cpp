#include "igpch.h"
#include "LayerStack.h"

namespace Ignis
{

LayerStack::~LayerStack()
{
    clear();
}

void LayerStack::clear()
{
    for (Layer* layer : m_layers)
    {
        layer->detach();
        delete layer;
    }
    m_layers.clear();
    m_layers_insert_index = 0;
}

void LayerStack::push_layer(Layer* layer)
{
    m_layers.emplace(m_layers.begin() + m_layers_insert_index, layer);
    m_layers_insert_index++;
}

void LayerStack::push_overlay(Layer* overlay)
{
    m_layers.emplace_back(overlay);
}

void LayerStack::pop_layer(Layer* layer)
{
    auto it = std::find(m_layers.begin(), m_layers.begin() + m_layers_insert_index, layer);
    if (it != m_layers.begin() + m_layers_insert_index)
    {
        layer->detach();
        m_layers.erase(it);
        m_layers_insert_index--;
    }
}

void LayerStack::pop_overlay(Layer* overlay)
{
    auto it = std::find(m_layers.begin() + m_layers_insert_index, m_layers.end(), overlay);
    if (it != m_layers.end())
    {
        overlay->detach();
        m_layers.erase(it);
    }
}

} // namespace Ignis
