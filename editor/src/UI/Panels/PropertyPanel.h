#pragma once

#include "IPanel.h"

namespace Ignis
{

class SceneEditorData;

class PropertyPanel : public IPanel
{
private:
    char m_component_search[128] = "";
    void draw_add_component_popup(SceneEditorData* data, Entity& selected);

public:
    PropertyPanel() = default;

    PanelId     get_id() const override;
    const char* get_title() const override
    {
        return "Properties";
    }
    void draw(IWorkspaceData* ctx) override;
};

} // namespace Ignis
