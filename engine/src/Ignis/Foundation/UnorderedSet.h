#pragma once

#include <unordered_set>

namespace Ignis
{
    template<
        typename T,
        typename Hash      = std::hash<T>,
        typename KeyEqual  = std::equal_to<T>,
        typename Allocator = std::allocator<T>>
    using UnorderedSet = std::unordered_set<T, Hash, KeyEqual, Allocator>;
}
