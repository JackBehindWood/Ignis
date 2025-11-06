#pragma once

#include <Ignis/Math/MathUtils.h>

#ifdef IGNIS_DEBUG
#define IGNIS_MEMORY_DEBUG
#endif

namespace Ignis
{
    class MemStack
    {
    private:
        struct MemBlock
        {
            MemBlock* prev;
            size_t size;
            size_t used;
            void* data;

            MemBlock(size_t block_size)
                : prev(nullptr), size(block_size), used(0)
            {
                data = malloc(size);
            }

            ~MemBlock()
            {
                ::free(data);
            }
        };

        MemBlock* m_top;
    public:
        explicit MemStack(size_t initial_block_size = 1024)
            : m_top(new MemBlock(initial_block_size))
        {
        }
        
        ~MemStack()
        {
            reset();

        }

        MemStack(const MemStack&) = delete;
        MemStack(MemStack&& other) noexcept
        {
            m_top = other.m_top;
            other.m_top = nullptr;
        }

        MemStack& operator=(const MemStack&) = delete;

        MemStack& operator=(MemStack&& other) = delete;

        void* allocate(size_t size)
        {
            if (m_top->used + size > m_top->size)
            {
                size_t block_size = Math::max(size, m_top->size);
                MemBlock* new_block = new MemBlock(block_size);
                new_block->prev = m_top;
                m_top = new_block;
            }

            void* ptr = static_cast<char*>(m_top->data) + m_top->used;
            m_top->used += size;
            return ptr;
        }

        void* allocate(size_t size, size_t alignment)
        {
            // Ensure alignment is a power of 2
            IG_CORE_ASSERT((alignment & (alignment - 1)) == 0, "alignment must be a power of 2!");

            size_t current_offset = reinterpret_cast<size_t>(m_top->data) + m_top->used;
            size_t aligned_offset = (current_offset + alignment - 1) & ~(alignment - 1);
            size_t padding = aligned_offset - current_offset;

            if (m_top->used + size + padding > m_top->size)
            {
                size_t block_size = Math::max(size + padding, m_top->size);
                MemBlock* new_block = new MemBlock(block_size);
                new_block->prev = m_top;
                m_top = new_block;

                current_offset = reinterpret_cast<size_t>(m_top->data);
                aligned_offset = (current_offset + alignment - 1) & ~(alignment - 1);
                padding = aligned_offset - current_offset;
            }

            void* ptr = reinterpret_cast<void*>(reinterpret_cast<size_t>(m_top->data) + m_top->used + padding);
            m_top->used += size + padding;
            return ptr;
        }

        void reset()
        {
            while (m_top)
            {
                MemBlock* temp = m_top;
                m_top = temp->prev;
                delete temp;
            }
            m_top = nullptr;
        }

        size_t get_total_allocated() const 
        { 
            size_t count = 0;
            for (MemBlock* block = m_top; block; block = block->prev)
            {
                count += block->used;
            }
            return count;
        }

        inline bool empty() const { return m_top == nullptr; }
    };
}