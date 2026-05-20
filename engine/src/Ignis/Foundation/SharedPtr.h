#pragma once
#include "RefCounted.h"

namespace Ignis
{
    template<typename T>
    class SharedPtr
    {
        template<typename U> friend class SharedPtr;
    public:
        constexpr SharedPtr() = default;

        constexpr SharedPtr(T* ptr) : m_ptr(ptr)
        {
            if (m_ptr) m_ptr->add_ref();
        }

        constexpr SharedPtr(std::nullptr_t) noexcept : m_ptr(nullptr) {}

        constexpr SharedPtr(const SharedPtr& other) : m_ptr(other.m_ptr)
        {
            if (m_ptr) m_ptr->add_ref();
        }

        template<typename U>
        constexpr SharedPtr(const SharedPtr<U>& other) : m_ptr(other.get()) 
        {
            if (m_ptr) m_ptr->add_ref();
        }

        constexpr SharedPtr(SharedPtr&& other) noexcept : m_ptr(other.m_ptr)
        {
            other.m_ptr = nullptr;
        }

        template<typename U>
        constexpr SharedPtr(SharedPtr<U>&& other) noexcept : m_ptr(other.release()) {}

        constexpr ~SharedPtr() { if (m_ptr) m_ptr->release(); }

        constexpr SharedPtr& operator=(const SharedPtr& other)
        {
            if (this != &other)
            {
                if (m_ptr) m_ptr->release();
                m_ptr = other.m_ptr;
                if (m_ptr) m_ptr->add_ref();
            }
            return *this;
        }

        template<typename U>
        constexpr SharedPtr& operator=(const SharedPtr<U>& other)
        {
            if (m_ptr) m_ptr->release();
            m_ptr = other.get();
            if (m_ptr) m_ptr->add_ref();
            return *this;
        }

        constexpr SharedPtr& operator=(SharedPtr&& other) noexcept
        {
            if (this != &other)
            {
                if (m_ptr) m_ptr->release();
                m_ptr = other.m_ptr;
                other.m_ptr = nullptr;
            }
            return *this;
        }

        template<typename U>
        constexpr SharedPtr& operator=(SharedPtr<U>&& other) noexcept
        {
            if (m_ptr) m_ptr->release();
            m_ptr = other.release();
            return *this;
        }

        constexpr SharedPtr& operator=(std::nullptr_t) noexcept
        {
            if (m_ptr) m_ptr->release();
            m_ptr = nullptr;
            return *this;
        }

        constexpr T* operator->() const { return m_ptr; }
        constexpr T& operator*()  const { return *m_ptr; }
        constexpr T* get()        const { return m_ptr; }
        constexpr explicit operator bool() const { return m_ptr != nullptr; }

        constexpr bool operator==(const SharedPtr& other) const { return m_ptr == other.m_ptr; }
        constexpr bool operator!=(const SharedPtr& other) const { return m_ptr != other.m_ptr; }

    private:
        constexpr T* release() noexcept
        {
            T* ptr = m_ptr;
            m_ptr = nullptr;
            return ptr;
        }

        T* m_ptr = nullptr;
    };

    template<typename T, typename... Args>
    constexpr SharedPtr<T> create_shared(Args&&... args)
    {
        return SharedPtr<T>(new T(static_cast<Args&&>(args)...));
    }

    // Checked downcast: SharedPtr<Base> → SharedPtr<Derived>.
    // Mirrors std::static_pointer_cast — use only when the stored type is known to be T.
    template<typename T, typename U>
    constexpr SharedPtr<T> static_pointer_cast(const SharedPtr<U>& other)
    {
        return SharedPtr<T>(static_cast<T*>(other.get()));
    }

    template<typename T>
    using WeakPtr = T*;
}