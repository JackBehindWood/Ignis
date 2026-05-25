#pragma once

#include "String.h"
#include "Filesystem.h"

#include <string_view>

namespace Ignis
{
// 64-bit FNV-1a hash of a file path string — used as a zero-allocation key for
// path-indexed maps. Replaces dynamic String keys in hot lookup paths.
//
// In IG_DEBUG builds, the raw string is retained for editor tooltips and collision
// assertions. In release builds, only the 64-bit hash is stored.
struct RuntimePathId
{
    uint64_t hash = 0;

#if defined(IG_DEBUG)
    String debug_path;
#endif

    RuntimePathId() = default;

    explicit RuntimePathId(std::string_view sv)
        : hash(fnv1a(sv))
#if defined(IG_DEBUG)
          ,
          debug_path(sv)
#endif
    {
    }

    explicit RuntimePathId(const Path& p)
        : RuntimePathId(std::string_view(p.string()))
    {
    }

    bool operator==(const RuntimePathId& o) const noexcept
    {
        return hash == o.hash;
    }
    bool operator!=(const RuntimePathId& o) const noexcept
    {
        return hash != o.hash;
    }

    bool valid() const noexcept
    {
        return hash != 0;
    }

    static uint64_t fnv1a(std::string_view sv) noexcept
    {
        uint64_t h = 14695981039346656037ULL;
        for (unsigned char c : sv)
        {
            h ^= static_cast<uint64_t>(c);
            h *= 1099511628211ULL;
        }
        return h;
    }
};

} // namespace Ignis

namespace std
{
template <>
struct hash<Ignis::RuntimePathId>
{
    size_t operator()(const Ignis::RuntimePathId& id) const noexcept
    {
        return static_cast<size_t>(id.hash);
    }
};
} // namespace std
