#pragma once
#include "InputTypes.h"
#include <Ignis/Foundation/Foundation.h>

namespace Ignis
{

class InputContext
{
public:
    explicit InputContext(const char* name, int32_t priority = 0);

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
    struct Mapping
    {
        ActionID     action;
        InputBinding binding;
    };

    const char*     m_name;
    int32_t         m_priority;
    bool            m_active = true;
    Vector<Mapping> m_mappings;

    friend class InputSystem;
};

} // namespace Ignis
