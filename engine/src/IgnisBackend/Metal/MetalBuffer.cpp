#include "igpch.h"
#include "MetalResource.h"

#include <Metal/Metal.hpp>

namespace Ignis
{
MetalBuffer::MetalBuffer(MTL::Buffer* buffer, uint32_t size)
    : m_buffer(buffer),
      m_size(size)
{
}

MetalBuffer::~MetalBuffer()
{
    if (m_buffer)
    {
        m_buffer->release();
    }
}

void MetalBuffer::bind_storage(MTL::ComputeCommandEncoder* encoder, uint32_t slot) const
{
    encoder->setBuffer(m_buffer, 0, slot);
}
} // namespace Ignis
