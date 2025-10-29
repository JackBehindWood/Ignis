#pragma once

namespace Ignis
{
    template<typename T>
    class MetalPtr;

    template<typename T, typename... Args>
    inline MetalPtr<T> make_metal(Args&&... args);

    template<typename T>
    class MetalPtr 
    {
    private:
        T* m_ptr;
    public:
        inline MetalPtr() : m_ptr(nullptr) {}
        inline explicit MetalPtr(T* ptr) : m_ptr(ptr) {}
        inline MetalPtr(const MetalPtr& other) : m_ptr(other.m_ptr) 
        {
            if (m_ptr) m_ptr->retain();
        }
        inline MetalPtr(MetalPtr&& other) noexcept : m_ptr(other.m_ptr) 
        {
            other.m_ptr = nullptr;
        }
        inline ~MetalPtr() 
        {
            if (m_ptr) m_ptr->release();
        }

        inline MetalPtr& operator=(T* ptr)
        {
            if (m_ptr) m_ptr->release();
            m_ptr = ptr;
            if (m_ptr) m_ptr->retain();
            return *this;
        }

        inline MetalPtr& operator=(const MetalPtr& other) 
        {
            if (this != &other) 
            {
                if (m_ptr) m_ptr->release();
                m_ptr = other.m_ptr;
                if (m_ptr) m_ptr->retain();
            }
            return *this;
        }

        inline MetalPtr& operator=(MetalPtr&& other) noexcept 
        {
            if (this != &other) 
            {
                if (m_ptr) m_ptr->release();
                m_ptr = other.m_ptr;
                other.m_ptr = nullptr;
            }
            return *this;
        }

        [[nodiscard]] inline T* get() const { return m_ptr; }
        [[nodiscard]] inline T** get_address() { return &m_ptr; }
        [[nodiscard]] inline T* operator->() const { return m_ptr; }
        [[nodiscard]] inline operator T*() const { return m_ptr; }

        inline bool operator==(const MetalPtr& other) const { return m_ptr == other.m_ptr; }
        inline bool operator!=(const MetalPtr& other) const { return m_ptr != other.m_ptr; }
        inline bool operator==(std::nullptr_t) const { return m_ptr == nullptr; }
        inline bool operator!=(std::nullptr_t) const { return m_ptr != nullptr; }

        inline void reset(T* ptr = nullptr) 
        {
            if (m_ptr) m_ptr->release();
            m_ptr = ptr;
            if (m_ptr) m_ptr->retain();
        }

        inline void swap(MetalPtr& other) noexcept
        { 
            std::swap(m_ptr, other.m_ptr); 
        }

        inline T* detach() 
        {
            T* tmp = m_ptr;
            m_ptr = nullptr;
            return tmp;
        }

        inline T** release_and_get_address_of()
        {
            if (m_ptr) 
            {
                m_ptr->release();
                m_ptr = nullptr;
            }
            return &m_ptr;
        }

        static inline MetalPtr<T> from_retained(T* ptr)
        {
            MetalPtr<T> mp;
            mp.m_ptr = ptr; // take ownership directly
            return mp;
        }

        template<typename F>
        static inline MetalPtr<T> adopt(F&& creator) 
        {
            return MetalPtr<T>(creator());
        }
    };

    template<typename T, typename... Args>
    inline MetalPtr<T> make_metal(Args&&... args) 
    {
        return MetalPtr<T>(T::alloc()->init(std::forward<Args>(args)...));
    }
}