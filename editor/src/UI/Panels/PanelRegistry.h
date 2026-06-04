#pragma once

#include "IPanel.h"

namespace Ignis
{

struct PanelDescriptor
{
    PanelId     id;
    const char* title;
    UniquePtr<IPanel> (*factory)();
};

class PanelRegistry
{
public:
    void                           register_panel(PanelDescriptor desc);
    UniquePtr<IPanel>              create(PanelId id) const;
    const PanelDescriptor*         find(PanelId id) const;
    const Vector<PanelDescriptor>& all() const
    {
        return m_descs;
    }

private:
    Vector<PanelDescriptor> m_descs;
};

} // namespace Ignis
