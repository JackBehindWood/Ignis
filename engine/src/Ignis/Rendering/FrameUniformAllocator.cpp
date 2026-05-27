#include "igpch.h"
#include "FrameUniformAllocator.h"

#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/GRI/GRI.h"

namespace Ignis
{
void FrameUniformAllocator::init(uint32_t total_size_bytes)
{
    IG_CORE_ASSERT(total_size_bytes > 0, "FrameUniformAllocator: size must be non-zero");
    m_total_size = total_size_bytes;

    GRIBufferDesc desc;
    desc.size     = total_size_bytes;
    desc.usage    = GRIBufferUsage::UniformBuffer;
    m_ring_buffer = RenderSystem::get_gri()->create_buffer(desc);

    m_mapped_ptr = static_cast<uint8_t*>(m_ring_buffer->get_mapped_data());
    IG_CORE_ASSERT(m_mapped_ptr, "FrameUniformAllocator: buffer not CPU-accessible");
}

void FrameUniformAllocator::shutdown()
{
    m_ring_buffer = nullptr;
    m_mapped_ptr  = nullptr;
    m_total_size  = 0;
    m_head        = 0;
    m_frame_start = 0;
}

void FrameUniformAllocator::begin_frame()
{
    m_frame_start = 0;
    m_head        = 0;
}

void FrameUniformAllocator::end_frame()
{
    // max_frames_in_flight = 1: no GPU fence sync needed.
}

FrameUniformAllocator::Allocation FrameUniformAllocator::allocate(const void* data, uint32_t size)
{
    uint32_t aligned_offset = (m_head + (k_alignment - 1)) & ~(k_alignment - 1);
    IG_CORE_ASSERT(aligned_offset + size <= m_total_size, "FrameUniformAllocator: ring buffer overflow");

    memcpy(m_mapped_ptr + aligned_offset, data, size);
    m_head = aligned_offset + size;

    return {m_ring_buffer.get(), aligned_offset};
}
} // namespace Ignis
