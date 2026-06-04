#pragma once

#include "WorkspaceDefinition.h"
#include "../Panels/IPanel.h"

namespace Ignis
{

class IWorkspace
{
public:
    virtual ~IWorkspace() = default;

    virtual const WorkspaceDefinition&       definition() const        = 0;
    virtual IWorkspaceData*                  data()                    = 0;
    virtual void                             draw_menu_bar()           = 0;
    virtual void                             update(Timestep ts)       = 0;
    virtual const Vector<UniquePtr<IPanel>>& get_active_panels() const = 0;

    virtual bool has_pending_resize() const
    {
        return false;
    }
    virtual void flush_resize()
    {
    }
};

// WorkspaceBase owns the panel collection so concrete workspaces only
// override definition(), data(), draw_menu_bar(), and update().
class WorkspaceBase : public IWorkspace
{
public:
    void update(Timestep ts) override
    {
        for (auto& panel : m_panels)
        {
            panel->update(ts.get_seconds(), data());
        }
    }

    const Vector<UniquePtr<IPanel>>& get_active_panels() const override
    {
        return m_panels;
    }

protected:
    Vector<UniquePtr<IPanel>> m_panels;
};

} // namespace Ignis
