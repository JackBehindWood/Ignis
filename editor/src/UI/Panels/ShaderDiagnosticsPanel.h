#pragma once

#include "IPanel.h"

namespace Ignis
{

class ShaderDiagnosticsPanel : public IPanel
{
public:
    PanelId     get_id() const override;
    const char* get_title() const override
    {
        return "Shader Diagnostics";
    }
    void draw(IWorkspaceData* ctx) override;

private:
    String m_evict_status;
};

} // namespace Ignis
