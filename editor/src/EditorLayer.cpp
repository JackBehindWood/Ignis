#include "igpch.h"
#include "EditorLayer.h"

namespace Ignis 
{

    EditorLayer::EditorLayer()
        : Layer("EditorLayer")
    {
    }

    void EditorLayer::attach()
    {
        // Initialization code here
        IG_INFO("EditorLayer attached");
    }

    void EditorLayer::detach()
    {
        // Cleanup code here
        IG_INFO("EditorLayer detached");
    }

    void EditorLayer::update(Timestep ts)
    {
        // Update logic here
        //IG_INFO("EditorLayer updated: {0} seconds", ts.get_seconds());
    }

    void EditorLayer::event(Event& event)
    {
        // Event handling code here
    }
}