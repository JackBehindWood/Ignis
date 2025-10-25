#pragma once

#include <memory>

namespace Ignis
{
    template<typename T>
    using UniquePtr = std::unique_ptr<T>;

    template<typename T, typename... Args>
    constexpr UniquePtr<T> create_unique(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    template<typename T>
    using SharedPtr = std::shared_ptr<T>;

    template<typename T, typename... Args>
    constexpr SharedPtr<T> create_shared(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    using WeakPtr = std::weak_ptr<T>;
}
