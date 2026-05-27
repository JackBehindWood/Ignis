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
} // namespace Ignis
