#include "igpch.h"
#include "MetalBindlessArray.h"
#include "MetalResource.h"

#include <Metal/Metal.hpp>

namespace Ignis
{

// [[id(N)]] slot assignments inside the SPIRV-Cross Tier2 argument buffer struct.
// g_material_textures[] at binding=0 → [[id(0)]]
// g_material_samplers[] at binding=0 → [[id(1)]] (next slot after texture)
// Verify against /tmp/ig_*.metal debug output if layout ever changes.
static constexpr NS::UInteger k_texture_id = 0;
static constexpr NS::UInteger k_sampler_id = 1;

MetalBindlessArray::~MetalBindlessArray()
{
    if (m_encoder)
    {
        m_encoder->release();
        m_encoder = nullptr;
    }
    if (m_arg_buf)
    {
        m_arg_buf->release();
        m_arg_buf = nullptr;
    }
}

void MetalBindlessArray::init(MTL::Device* device, MTL::Function* ps_function)
{
    IG_CORE_ASSERT(device, "MetalBindlessArray::init: null device");
    IG_CORE_ASSERT(ps_function, "MetalBindlessArray::init: null PS function");

    m_device = device;

    if (m_encoder)
    {
        m_encoder->release();
    }
    // Derive argument encoder from [[buffer(0)]] (space0) of the PBR PS function.
    // The encoder captures the exact Tier2 struct layout emitted by SPIRV-Cross.
    m_encoder = ps_function->newArgumentEncoder(0);
    IG_CORE_ASSERT(m_encoder, "MetalBindlessArray::init: newArgumentEncoder(0) returned null");

    m_stride = m_encoder->encodedLength();
    IG_CORE_ASSERT(m_stride > 0, "MetalBindlessArray::init: argument encoder stride is zero");

    grow(k_initial_capacity);
}

uint32_t MetalBindlessArray::register_texture(GRITexture2DPtr texture, GRISamplerStatePtr sampler)
{
    IG_CORE_ASSERT(texture, "MetalBindlessArray::register_texture: null texture");
    IG_CORE_ASSERT(sampler, "MetalBindlessArray::register_texture: null sampler");

    const uint32_t idx = static_cast<uint32_t>(m_entries.size());

    auto* mt = static_cast<MetalTexture2D*>(texture.get());
    m_tex_ptrs.push_back(mt->get_texture());
    m_entries.push_back({std::move(texture), std::move(sampler)});
    m_dirty = true;
    return idx;
}

void MetalBindlessArray::encode(MTL::RenderCommandEncoder* enc)
{
    if (!m_encoder || m_entries.empty())
    {
        return;
    }

    if (m_dirty)
    {
        rebuild();
    }

    // Bind the argument buffer at slot 0 for both shader stages (HLSL space0 → Metal [[buffer(0)]]).
    enc->setVertexBuffer(m_arg_buf, 0, 0);
    enc->setFragmentBuffer(m_arg_buf, 0, 0);

    // Single batched residency declaration — not per-draw.
    enc->useResources(reinterpret_cast<const MTL::Resource* const*>(m_tex_ptrs.data()),
                      static_cast<NS::UInteger>(m_tex_ptrs.size()), MTL::ResourceUsageSample, MTL::RenderStageFragment);
}

void MetalBindlessArray::clear()
{
    m_entries.clear();
    m_tex_ptrs.clear();
    m_dirty = true;
}

void MetalBindlessArray::grow(uint32_t new_capacity)
{
    IG_CORE_ASSERT(m_encoder, "MetalBindlessArray::grow: encoder not initialized");

    if (m_arg_buf)
    {
        m_arg_buf->release();
    }
    m_arg_buf = m_device->newBuffer(static_cast<NS::UInteger>(new_capacity) * m_stride, MTL::ResourceStorageModeShared);
    IG_CORE_ASSERT(m_arg_buf, "MetalBindlessArray::grow: buffer allocation failed");

    m_capacity = new_capacity;
    m_dirty    = true;
}

void MetalBindlessArray::rebuild()
{
    const uint32_t n = static_cast<uint32_t>(m_entries.size());

    if (n > m_capacity)
    {
        grow(n * 2);
    }

    for (uint32_t i = 0; i < n; ++i)
    {
        m_encoder->setArgumentBuffer(m_arg_buf, 0, static_cast<NS::UInteger>(i));

        auto* mt = static_cast<MetalTexture2D*>(m_entries[i].texture.get());
        auto* ms = static_cast<MetalSamplerState*>(m_entries[i].sampler.get());
        m_encoder->setTexture(mt->get_texture(), k_texture_id);
        m_encoder->setSamplerState(ms->get_sampler(), k_sampler_id);
    }

    m_dirty = false;
}

} // namespace Ignis
