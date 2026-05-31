#pragma once

#include <Ignis.h>

namespace Ignis
{

inline bool str_icontains(const char* haystack, const char* needle)
{
    if (!needle || needle[0] == '\0')
    {
        return true;
    }
    if (!haystack)
    {
        return false;
    }
    String h(haystack), n(needle);
    auto   to_lower = [](String s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    };
    return to_lower(h).find(to_lower(n)) != String::npos;
}

} // namespace Ignis
