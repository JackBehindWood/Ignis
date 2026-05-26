#pragma once
#include <atomic>

namespace Ignis
{
template <typename T>
using Atomic = std::atomic<T>;

template <typename T>
class RelaxedAtomic
{
public:
    RelaxedAtomic() = default;
    explicit RelaxedAtomic(T v)
        : m_value(v)
    {
    }

    T load() const
    {
        return m_value.load(std::memory_order_relaxed);
    }
    void store(T v)
    {
        m_value.store(v, std::memory_order_relaxed);
    }

    operator T() const
    {
        return load();
    }
    RelaxedAtomic& operator=(T v)
    {
        store(v);
        return *this;
    }

private:
    std::atomic<T> m_value{};
};
} // namespace Ignis
