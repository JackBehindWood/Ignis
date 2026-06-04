#pragma once
#include "InputTypes.h"
#include <Ignis/Foundation/Foundation.h>

namespace Ignis
{

class InputContext
{
public:
    explicit InputContext(const char* name, int32_t priority = 0);

    constexpr InputContext(const char* name, const InputMapping* static_mappings, size_t count,
                           int32_t priority = 0) noexcept
        : m_name(name),
          m_priority(priority),
          m_static_mappings(static_mappings),
          m_mapping_count(count),
          m_is_static(true)
    {
    }

    InputContext& map_action(ActionID action, uint32_t hw_code, Modifier mods = Modifier::None,
                             ModifierPolicy policy = ModifierPolicy::Exact, float scale = 1.0f);

    void set_active(bool v)
    {
        m_active = v;
    }
    bool is_active() const
    {
        return m_active;
    }
    int32_t priority() const
    {
        return m_priority;
    }
    const char* name() const
    {
        return m_name;
    }

private:
    const char*          m_name;
    int32_t              m_priority;
    bool                 m_active    = true;
    bool                 m_is_static = false;
    Vector<InputMapping> m_mappings;
    const InputMapping*  m_static_mappings = nullptr;
    size_t               m_mapping_count   = 0;

    friend class InputSystem;
};

} // namespace Ignis
