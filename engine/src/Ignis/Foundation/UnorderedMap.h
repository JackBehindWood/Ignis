#pragma once 

#include <unordered_map>

namespace Ignis
{
    template<typename T, typename Q>
    using UnorderedMap = std::unordered_map<T, Q>;
}