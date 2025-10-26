#include "igpch.h"
#include "MetalViewport.h"

namespace Ignis
{
    MetalViewport::MetalViewport(uint32_t width, uint32_t height) : m_width(width), m_height(height)
    {
    }

    MetalViewport::~MetalViewport()
    {
    }

    void MetalViewport::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
    }

    uint32_t MetalViewport::get_width() const
    {
        return m_width;
    }

    uint32_t MetalViewport::get_height() const
    {
        return m_height;
    }
} // namespace Ignis
