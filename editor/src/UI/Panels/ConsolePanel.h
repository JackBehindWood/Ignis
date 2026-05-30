#pragma once

#include "IPanel.h"
#include "Logging/ConsoleSink.h"

namespace Ignis
{

class ConsolePanel : public IPanel
{
public:
    PanelId     get_id() const override;
    const char* get_title() const override
    {
        return "Console";
    }
    void draw(IWorkspaceData* ctx) override;

private:
    bool m_filter[6]   = {true, true, true, true, true, true};
    bool m_auto_scroll = true;
};

} // namespace Ignis
