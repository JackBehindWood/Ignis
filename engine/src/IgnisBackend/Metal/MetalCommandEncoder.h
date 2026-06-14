#pragma once

#include <Metal/Metal.hpp>

namespace Ignis
{
class MetalRenderCommandEncoder
{
private:
    MTL::RenderCommandEncoder* m_encoder;

public:
    MetalRenderCommandEncoder(MTL::RenderCommandEncoder* encoder)
        : m_encoder(encoder)
    {
        m_encoder->retain();
    }

    ~MetalRenderCommandEncoder()
    {
        m_encoder->endEncoding();
        m_encoder->release();
    }

    inline MTL::RenderCommandEncoder* get() const
    {
        return m_encoder;
    }
};

class MetalComputeCommandEncoder
{
private:
    MTL::ComputeCommandEncoder* m_encoder;

public:
    MetalComputeCommandEncoder(MTL::ComputeCommandEncoder* encoder)
        : m_encoder(encoder)
    {
        m_encoder->retain();
    }

    ~MetalComputeCommandEncoder()
    {
        m_encoder->endEncoding();
        m_encoder->release();
    }

    inline MTL::ComputeCommandEncoder* get() const
    {
        return m_encoder;
    }
};
} // namespace Ignis
