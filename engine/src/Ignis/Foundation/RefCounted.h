#pragma once
#include <cstdint>
#include <atomic>

namespace Ignis
{

class RefCounted
{
public:
    void add_ref() const
    {
        ++m_ref_count;
    }
    void release() const
    {
        if (--m_ref_count == 0)
        {
            delete this;
        }
    }
    uint32_t ref_count() const
    {
        return m_ref_count.load();
    }

protected:
    RefCounted()          = default;
    virtual ~RefCounted() = default;

private:
    mutable std::atomic<uint32_t> m_ref_count{0};
};
} // namespace Ignis