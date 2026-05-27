#pragma once

#include "Ignis/Rendering/GRI/GRIResource.h"

namespace Ignis
{
class FrameUniformAllocator
{
public:
    void init(uint32_t total_size_bytes);
    void shutdown();

    void begin_frame();
    void end_frame();

    struct Allocation
    {
        GRIBuffer* buffer;
        uint32_t   offset;
    };

    Allocation allocate(const void* data, uint32_t size);

private:
    static constexpr uint32_t k_alignment = 256;

    GRIBufferPtr m_ring_buffer;
    uint8_t*     m_mapped_ptr  = nullptr;
    uint32_t     m_total_size  = 0;
    uint32_t     m_head        = 0;
    uint32_t     m_frame_start = 0;
};
} // namespace Ignis
