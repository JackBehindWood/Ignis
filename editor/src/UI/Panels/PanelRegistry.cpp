#include "edpch.h"
#include "PanelRegistry.h"

namespace Ignis
{

void PanelRegistry::register_panel(PanelDescriptor desc)
{
    IG_ASSERT(find(desc.id) == nullptr, "Panel ID already registered");
    m_descs.push_back(desc);
}

UniquePtr<IPanel> PanelRegistry::create(PanelId id) const
{
    const PanelDescriptor* desc = find(id);
    IG_ASSERT(desc, "PanelRegistry::create — unknown panel id");
    return desc->factory();
}

const PanelDescriptor* PanelRegistry::find(PanelId id) const
{
    for (const PanelDescriptor& d : m_descs)
    {
        if (d.id == id)
        {
            return &d;
        }
    }
    return nullptr;
}

} // namespace Ignis
