#pragma once

#include <Ignis/Foundation/Vector.h>
#include <Ignis/Rendering/GRI/GRIResource.h>
#include <Ignis/Rendering/GRI/GRIDefinitions.h>
#include <Ignis/Rendering/RenderGraph/RGResource.h>

namespace Ignis
{

class RenderGraphResourcePool
{
public:
    GRITexture2D* acquire( const GRITexture2DDesc& desc, uint16_t first_used, uint16_t last_used, uint32_t current_frame);

    GRIBuffer*    acquire_buffer(const GRIBufferDesc& desc, uint16_t first_used, uint16_t last_used, uint32_t current_frame);

    void begin_frame(uint32_t current_frame);

private:
    static constexpr uint32_t k_eviction_age = 4;

    struct Interval { uint16_t first; uint16_t last; };

    struct TextureEntry
    {
        GRITexture2DPtr  resource;
        GRITexture2DDesc desc            = {};
        uint32_t         last_frame_used = 0;
        Vector<Interval> committed;
    };

    struct BufferEntry
    {
        GRIBufferPtr     resource;
        GRIBufferDesc    desc            = {};
        uint32_t         last_frame_used = 0;
        Vector<Interval> committed;
    };

    Vector<TextureEntry> m_texture_entries;
    Vector<BufferEntry>  m_buffer_entries;
};

} // namespace Ignis
