#pragma once 

#include <unordered_map>

namespace Ignis
{
    template<typename T, typename Q, typename Hash = std::hash<T>, typename KeyEqual = std::equal_to<T>, typename Allocator = std::allocator<std::pair<const T, Q>>>
    using UnorderedMap = std::unordered_map<T, Q, Hash, KeyEqual, Allocator>;
}