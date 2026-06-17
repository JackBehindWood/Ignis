#pragma once

#include "IPanel.h"

namespace Ignis
{

class IBLStudioPanel : public IPanel
{
public:
    PanelId     get_id() const override;
    const char* get_title() const override
    {
        return "IBL Studio";
    }
    void update(float ts, IWorkspaceData* ctx) override;
    void draw(IWorkspaceData* ctx) override;

private:
    enum class BakeState
    {
        Idle,
        Baking,
        Done,
        Failed
    };

    char      m_hdr_path[512] = {};
    BakeState m_state         = BakeState::Idle;
    String    m_status        = "Idle";
    bool      m_pending_bake  = false;
};

} // namespace Ignis
