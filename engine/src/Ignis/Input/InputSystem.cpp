#include "igpch.h"
#include "InputSystem.h"

#include <Ignis/Core/Input.h>
#include <Ignis/Core/KeyCodes.h>
#include <Ignis/Core/MouseCodes.h>
#include <Ignis/Events/Event.h>
#include <Ignis/Events/KeyEvent.h>
#include <Ignis/Events/MouseEvent.h>

namespace Ignis
{

namespace Utils
{

static constexpr const char* k_letter_names[26] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
                                                   "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};
static constexpr const char* k_digit_names[10]  = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
static constexpr const char* k_fkey_names[25]   = {"F1",  "F2",  "F3",  "F4",  "F5",  "F6",  "F7",  "F8",  "F9",
                                                   "F10", "F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18",
                                                   "F19", "F20", "F21", "F22", "F23", "F24", "F25"};

constexpr const char* key_name(uint32_t hw_code) noexcept
{
    if (hw_code & 0xFFFF0000u)
    {
        switch (hw_code & 0xFFFFu)
        {
            case 0:
                return "LMB";
            case 1:
                return "RMB";
            case 2:
                return "MMB";
            default:
                return "Mouse";
        }
    }
    if (hw_code >= Key::A && hw_code <= Key::Z)
    {
        return k_letter_names[hw_code - Key::A];
    }
    if (hw_code >= Key::D0 && hw_code <= Key::D9)
    {
        return k_digit_names[hw_code - Key::D0];
    }
    if (hw_code >= Key::F1 && hw_code <= Key::F25)
    {
        return k_fkey_names[hw_code - Key::F1];
    }
    switch (hw_code)
    {
        case Key::Space:
            return "Space";
        case Key::Enter:
            return "Enter";
        case Key::Escape:
            return "Esc";
        case Key::Tab:
            return "Tab";
        case Key::Backspace:
            return "Bksp";
        case Key::Delete:
            return "Del";
        case Key::Insert:
            return "Ins";
        case Key::Left:
            return "Left";
        case Key::Right:
            return "Right";
        case Key::Up:
            return "Up";
        case Key::Down:
            return "Down";
        case Key::PageUp:
            return "PgUp";
        case Key::PageDown:
            return "PgDn";
        case Key::Home:
            return "Home";
        case Key::End:
            return "End";
        default:
            return "?";
    }
}

static constexpr FixedInputString<32> format_binding(const InputBinding& b)
{
    FixedInputString<32> s;
    if (has_mod(b.modifiers, Modifier::Ctrl))
    {
        s.append("Ctrl+");
    }
    if (has_mod(b.modifiers, Modifier::Super))
    {
#if defined(IG_PLATFORM_MACOS)
        s.append("Cmd+");
#else
        s.append("Win+");
#endif
    }
    if (has_mod(b.modifiers, Modifier::Alt))
    {
        s.append("Alt+");
    }
    if (has_mod(b.modifiers, Modifier::Shift))
    {
        s.append("Shift+");
    }
    s.append(key_name(b.hardware_code));
    return s;
}

} // namespace Utils

Vector<InputContext*>                            InputSystem::s_contexts;
UnorderedMap<ActionID, InputSystem::ActionState> InputSystem::s_frame_states;
UnorderedMap<ActionID, InputSystem::ActionState> InputSystem::s_pending_states;
bool                                             InputSystem::s_block_game_input = false;
float                                            InputSystem::s_scroll_delta     = 0.0f;

void InputSystem::register_context(InputContext* ctx)
{
    auto it = s_contexts.begin();
    while (it != s_contexts.end() && (*it)->priority() >= ctx->priority())
    {
        ++it;
    }
    s_contexts.insert(it, ctx);
}

void InputSystem::unregister_context(InputContext* ctx)
{
    auto it = std::find(s_contexts.begin(), s_contexts.end(), ctx);
    if (it != s_contexts.end())
    {
        s_contexts.erase(it);
    }
}

void InputSystem::begin_frame()
{
    s_scroll_delta = 0.0f;
    s_frame_states = s_pending_states;

    for (auto& [id, state] : s_pending_states)
    {
        state.state = state.state & ~(InputState::Started | InputState::Completed);
    }
}

Modifier InputSystem::snapshot_modifiers()
{
    uint8_t mods = 0;
    if (Input::is_key_pressed(Key::LeftShift) || Input::is_key_pressed(Key::RightShift))
    {
        mods |= static_cast<uint8_t>(Modifier::Shift);
    }
    if (Input::is_key_pressed(Key::LeftControl) || Input::is_key_pressed(Key::RightControl))
    {
        mods |= static_cast<uint8_t>(Modifier::Ctrl);
    }
    if (Input::is_key_pressed(Key::LeftAlt) || Input::is_key_pressed(Key::RightAlt))
    {
        mods |= static_cast<uint8_t>(Modifier::Alt);
    }
    if (Input::is_key_pressed(Key::LeftSuper) || Input::is_key_pressed(Key::RightSuper))
    {
        mods |= static_cast<uint8_t>(Modifier::Super);
    }
    return static_cast<Modifier>(mods);
}

void InputSystem::process_pressed(uint32_t hw_code)
{
    Modifier snapshot = snapshot_modifiers();

    for (InputContext* ctx : s_contexts)
    {
        if (!ctx->is_active())
        {
            continue;
        }
        if (s_block_game_input && ctx->priority() < k_game_priority_threshold)
        {
            continue;
        }
        const InputMapping* begin = ctx->m_is_static ? ctx->m_static_mappings : ctx->m_mappings.data();
        size_t              count = ctx->m_is_static ? ctx->m_mapping_count : ctx->m_mappings.size();
        for (size_t i = 0; i < count; ++i)
        {
            if (chord_matches(begin[i].binding, hw_code, snapshot))
            {
                auto& state = s_pending_states[begin[i].action];
                state.state = state.state | InputState::Started | InputState::Triggered;
                state.value = begin[i].binding.scale;
                return;
            }
        }
    }
}

void InputSystem::process_released(uint32_t hw_code)
{
    for (InputContext* ctx : s_contexts)
    {
        const InputMapping* begin = ctx->m_is_static ? ctx->m_static_mappings : ctx->m_mappings.data();
        size_t              count = ctx->m_is_static ? ctx->m_mapping_count : ctx->m_mappings.size();
        for (size_t i = 0; i < count; ++i)
        {
            if (begin[i].binding.hardware_code != hw_code)
            {
                continue;
            }
            auto it = s_pending_states.find(begin[i].action);
            if (it != s_pending_states.end() && has_state(it->second.state, InputState::Triggered))
            {
                it->second.state = (it->second.state & ~InputState::Triggered) | InputState::Completed;
                it->second.value = 0.0f;
            }
        }
    }
}

float InputSystem::get_scroll_delta()
{
    return s_scroll_delta;
}

void InputSystem::on_event(Event& e)
{
    EventDispatcher d(e);
    d.dispatch<MouseScrolledEvent>(
        [](MouseScrolledEvent& ev) -> bool
        {
            s_scroll_delta += ev.get_y_offset();
            return false;
        });
    d.dispatch<KeyPressedEvent>(
        [](KeyPressedEvent& ev) -> bool
        {
            InputSystem::process_pressed(static_cast<uint32_t>(ev.get_key_code()));
            return false;
        });
    d.dispatch<KeyReleasedEvent>(
        [](KeyReleasedEvent& ev) -> bool
        {
            InputSystem::process_released(static_cast<uint32_t>(ev.get_key_code()));
            return false;
        });
    d.dispatch<MouseButtonPressedEvent>(
        [](MouseButtonPressedEvent& ev) -> bool
        {
            InputSystem::process_pressed(mouse_code(ev.get_mouse_button()));
            return false;
        });
    d.dispatch<MouseButtonReleasedEvent>(
        [](MouseButtonReleasedEvent& ev) -> bool
        {
            InputSystem::process_released(mouse_code(ev.get_mouse_button()));
            return false;
        });
}

bool InputSystem::was_action_started(ActionID action)
{
    auto it = s_frame_states.find(action);
    return it != s_frame_states.end() && has_state(it->second.state, InputState::Started);
}

bool InputSystem::is_action_triggered(ActionID action)
{
    auto it = s_frame_states.find(action);
    return it != s_frame_states.end() && has_state(it->second.state, InputState::Triggered);
}

bool InputSystem::was_action_completed(ActionID action)
{
    auto it = s_frame_states.find(action);
    return it != s_frame_states.end() && has_state(it->second.state, InputState::Completed);
}

float InputSystem::get_axis_value(ActionID action)
{
    auto it = s_frame_states.find(action);
    return it != s_frame_states.end() ? it->second.value : 0.0f;
}

FixedInputString<32> InputSystem::get_action_label(ActionID action)
{
    for (InputContext* ctx : s_contexts)
    {
        const InputMapping* begin = ctx->m_is_static ? ctx->m_static_mappings : ctx->m_mappings.data();
        size_t              count = ctx->m_is_static ? ctx->m_mapping_count : ctx->m_mappings.size();
        for (size_t i = 0; i < count; ++i)
        {
            if (begin[i].action == action)
            {
                return Utils::format_binding(begin[i].binding);
            }
        }
    }
    return {};
}

void InputSystem::trigger_action(ActionID action)
{
    s_pending_states[action].state = s_pending_states[action].state | InputState::Started | InputState::Triggered;
    s_pending_states[action].value = 1.0f;
}

} // namespace Ignis
