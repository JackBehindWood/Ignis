#pragma once

#include <string>
#include <string_view>
#include <sstream>

namespace Ignis
{
using String       = std::string;
using StringView   = std::string_view;
using Stringstream = std::stringstream;

template <typename... Args>
String to_string(Args&&... args)
{
    return std::to_string(std::forward<Args>(args)...);
}
} // namespace Ignis