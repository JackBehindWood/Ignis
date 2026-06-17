#pragma once

#include "Ignis/Rendering/GRI/GRIResource.h"

#include <Metal/Metal.hpp>

namespace Ignis
{

class MetalBindlessArray
{
public:
    static constexpr uint32_t k_initial_capacity = 256;

    MetalBindlessArray() = default;
    ~MetalBindlessArray();

    MetalBindlessArray(const MetalBindlessArray&)            = delete;
    MetalBindlessArray& operator=(const MetalBindlessArray&) = delete;

    void     init(MTL::Device* device, MTL::Function* ps_function);
    uint32_t register_texture(GRITexture2DPtr texture, GRISamplerStatePtr sampler);
    void     encode(MTL::RenderCommandEncoder* enc);
    void     clear();

    bool is_ready() const
    {
        return m_encoder != nullptr;
    }
    uint32_t count() const
    {
        return static_cast<uint32_t>(m_entries.size());
    }

private:
    struct Entry
    {
        GRITexture2DPtr    texture;
        GRISamplerStatePtr sampler;
    };

    void grow(uint32_t new_capacity);
    void rebuild();

    Vector<Entry>         m_entries;
    Vector<MTL::Texture*> m_tex_ptrs; // raw ptrs for batched useResources — kept in sync with m_entries

    MTL::Device*          m_device   = nullptr;
    MTL::ArgumentEncoder* m_encoder  = nullptr;
    MTL::Buffer*          m_arg_buf  = nullptr;
    NS::UInteger          m_stride   = 0;
    uint32_t              m_capacity = 0;
    bool                  m_dirty    = true;
};

} // namespace Ignis
