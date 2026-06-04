#pragma once
#include <cstdint>

namespace Ignis
{

using ActionID = uint32_t;

enum class InputState : uint8_t
{
    None      = 0,
    Started   = 1 << 0,
    Triggered = 1 << 1,
    Completed = 1 << 2,
};

constexpr InputState operator|(InputState a, InputState b) noexcept
{
    return static_cast<InputState>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr InputState operator&(InputState a, InputState b) noexcept
{
    return static_cast<InputState>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
constexpr InputState operator~(InputState a) noexcept
{
    return static_cast<InputState>(~static_cast<uint8_t>(a));
}
constexpr bool has_state(InputState s, InputState bit) noexcept
{
    return (static_cast<uint8_t>(s) & static_cast<uint8_t>(bit)) != 0;
}

enum class Modifier : uint8_t
{
    None  = 0,
    Shift = 1 << 0,
    Ctrl  = 1 << 1,
    Alt   = 1 << 2,
    Super = 1 << 3,
};

constexpr Modifier operator|(Modifier a, Modifier b) noexcept
{
    return static_cast<Modifier>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr bool has_mod(Modifier mask, Modifier flag) noexcept
{
    return (static_cast<uint8_t>(mask) & static_cast<uint8_t>(flag)) != 0;
}

enum class ModifierPolicy : uint8_t
{
    Exact,    // snapshot == binding.modifiers — for shortcuts
    Agnostic, // (snapshot & binding.modifiers) == binding.modifiers — for movement
};

struct InputBinding
{
    uint32_t       hardware_code = 0;
    Modifier       modifiers     = Modifier::None;
    ModifierPolicy mod_policy    = ModifierPolicy::Exact;
    float          scale         = 1.0f;
};

struct InputMapping
{
    ActionID     action;
    InputBinding binding;
};

constexpr ActionID action_id(const char* s) noexcept
{
    ActionID h = 5381;
    while (*s)
    {
        h = ((h << 5) + h) ^ static_cast<uint8_t>(*s++);
    }
    return h;
}

template <size_t N>
struct FixedInputString
{
    char   data[N]{};
    size_t len = 0;

    constexpr void append(const char* str)
    {
        if (!str)
        {
            return;
        }
        while (*str && len < N - 1)
        {
            data[len++] = *str++;
        }
        data[len] = '\0';
    }

    constexpr const char* c_str() const
    {
        return data;
    }
    constexpr bool empty() const
    {
        return len == 0;
    }
};

} // namespace Ignis
