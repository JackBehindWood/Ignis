#pragma once

#include <string>
#include <sstream>

namespace Ignis
{
    using String = std::string;
    using Stringstream = std::stringstream;

    template <typename... Args>
    String to_string(Args&&... args)
    {
        return std::to_string(std::forward<Args>(args)...);
    }
}