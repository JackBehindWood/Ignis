#pragma once
#include <optional>

namespace Ignis
{
template <typename T>
using Optional = std::optional<T>;

inline constexpr std::nullopt_t NullOpt = std::nullopt;
} // namespace Ignis
