#include "igpch.h"
#include "InputContext.h"

namespace Ignis
{

InputContext::InputContext(const char* name, int32_t priority)
    : m_name(name),
      m_priority(priority)
{
}

InputContext& InputContext::map_action(ActionID action, uint32_t hw_code, Modifier mods, ModifierPolicy policy,
                                       float scale)
{
    m_mappings.push_back({action, {hw_code, mods, policy, scale}});
    return *this;
}

} // namespace Ignis
