#pragma once

#include <array>

namespace Ignis
{

    template <typename T, std::size_t N>
    using Array = std::array<T, N>; // Simple alias to avoid confusion with our Vector class and to allow easy future changes if needed

    

} // namespace Ignis