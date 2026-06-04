#pragma once

#include "IPanel.h"

namespace Ignis
{

class SceneTreePanel : public IPanel
{
public:
    SceneTreePanel() = default;

    PanelId     get_id() const override;
    const char* get_title() const override
    {
        return "Scene Tree";
    }
    void draw(IWorkspaceData* ctx) override;

private:
    void draw_entity_node(SceneEditorData* data, Entity& e);

    char m_search_buffer[256] = "";
};

} // namespace Ignis
