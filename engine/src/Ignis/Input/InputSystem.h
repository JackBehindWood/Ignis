#pragma once
#include "InputContext.h"

namespace Ignis
{
class Event;

class InputSystem
{
public:
    static void register_context(InputContext* ctx);
    static void unregister_context(InputContext* ctx);

    static void begin_frame();
    static void on_event(Event& e);

    static bool                 was_action_started(ActionID action);
    static bool                 is_action_triggered(ActionID action);
    static bool                 was_action_completed(ActionID action);
    static float                get_axis_value(ActionID action);
    static FixedInputString<32> get_action_label(ActionID action);

    static void trigger_action(ActionID action);

    static void set_block_game_input(bool block)
    {
        s_block_game_input = block;
    }

    static constexpr uint32_t mouse_code(uint16_t btn)
    {
        return 0x00010000u | btn;
    }

private:
    struct ActionState
    {
        InputState state = InputState::None;
        float      value = 0.0f;
    };

    static Vector<InputContext*>               s_contexts;
    static UnorderedMap<ActionID, ActionState> s_frame_states;
    static UnorderedMap<ActionID, ActionState> s_pending_states;
    static bool                                s_block_game_input;

    static constexpr int32_t k_game_priority_threshold = 50;

    static Modifier snapshot_modifiers();
    static void     process_pressed(uint32_t hw_code);
    static void     process_released(uint32_t hw_code);

    static constexpr bool chord_matches(const InputBinding& b, uint32_t hw_code, Modifier snapshot)
    {
        if (b.hardware_code != hw_code)
        {
            return false;
        }
        if (b.mod_policy == ModifierPolicy::Exact)
        {
            return b.modifiers == snapshot;
        }

        return (static_cast<uint8_t>(snapshot) & static_cast<uint8_t>(b.modifiers)) ==
               static_cast<uint8_t>(b.modifiers);
    }
};

} // namespace Ignis
