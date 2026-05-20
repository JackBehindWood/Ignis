#pragma once

namespace Ignis
{
    template<typename T>
    class UniquePtr
    {
        template<typename U> friend class UniquePtr;
    public:
        constexpr UniquePtr() = default;

        constexpr explicit UniquePtr(T* ptr) : m_ptr(ptr) {}
        constexpr UniquePtr(std::nullptr_t) noexcept : m_ptr(nullptr) {}

        constexpr UniquePtr(const UniquePtr&) = delete;
        constexpr UniquePtr& operator=(const UniquePtr&) = delete;

        constexpr UniquePtr(UniquePtr&& other) noexcept : m_ptr(other.m_ptr)
        {
            other.m_ptr = nullptr;
        }

        template<typename U>
        constexpr UniquePtr(UniquePtr<U>&& other) noexcept : m_ptr(other.release()) {}

        constexpr ~UniquePtr() { delete m_ptr; }

        constexpr UniquePtr& operator=(UniquePtr&& other) noexcept
        {
            if (this != &other)
            {
                delete m_ptr;
                m_ptr = other.m_ptr;
                other.m_ptr = nullptr;
            }
            return *this;
        }

        template<typename U>
        constexpr UniquePtr& operator=(UniquePtr<U>&& other) noexcept
        {
            delete m_ptr;
            m_ptr = other.release();
            return *this;
        }

        constexpr UniquePtr& operator=(std::nullptr_t) noexcept
        {
            reset(); // Safely deletes existing pointer and clears it
            return *this;
        }


        constexpr T* operator->() const { return m_ptr; }
        constexpr T& operator*()  const { return *m_ptr; }
        constexpr T* get()        const { return m_ptr; }
        constexpr explicit operator bool() const { return m_ptr != nullptr; }

        constexpr T* release()
        {
            T* ptr = m_ptr;
            m_ptr = nullptr;
            return ptr;
        }

        constexpr void reset(T* ptr = nullptr)
        {
            delete m_ptr;
            m_ptr = ptr;
        }

    private:
        T* m_ptr = nullptr;
    };

    template<typename T, typename... Args>
    constexpr UniquePtr<T> create_unique(Args&&... args)
    {
        return UniquePtr<T>(new T(static_cast<Args&&>(args)...));
    }
}