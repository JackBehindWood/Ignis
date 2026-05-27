#pragma once

#include <Ignis/Foundation/MemStack.h>
#include <Ignis/Foundation/Vector.h>
#include <Ignis/Rendering/GRI/GRIDefinitions.h>
#include <Ignis/Rendering/GRI/GRIResource.h>
#include "RGResource.h"
#include "RGPass.h"
#include "RGPool.h"

namespace Ignis
{

class GRICommandList;
class RGBuilder;

class RenderGraph
{
    friend class RGBuilder;

public:
    explicit RenderGraph(size_t arena_size = 64 * 1024);
    ~RenderGraph();

    void        reset();
    const char* intern_string(const char* src);

private:
    void compile();
    void fold_pass_into_fingerprint(const RGPassBase& pass, const char* name);

    RGTextureHandle                   register_texture(RGInternal::VirtualTexture&& vt);
    RGInternal::VirtualTexture&       get_virtual_texture(uint16_t id);
    const RGInternal::VirtualTexture& get_virtual_texture(uint16_t id) const;

    RGBufferHandle                   register_buffer(RGInternal::VirtualBuffer&& vb);
    RGInternal::VirtualBuffer&       get_virtual_buffer(uint16_t id);
    const RGInternal::VirtualBuffer& get_virtual_buffer(uint16_t id) const;

    GRIRenderPassInfo build_pass_info(const RGPassBase& pass) const;

    struct TopologyCache
    {
        uint64_t last_fingerprint = 0;
    };

    MemStack m_arena;
    uint32_t m_frame_index         = 0;
    uint64_t m_current_fingerprint = 0;

    Vector<RGPassBase*>                m_passes;
    Vector<RGInternal::VirtualTexture> m_textures;
    Vector<RGInternal::VirtualBuffer>  m_buffers;
    Vector<GRITexture2D*>              m_resolved_tex;
    Vector<GRIBuffer*>                 m_resolved_buf;
    Vector<uint16_t>                   m_sorted_passes;

    RenderGraphResourcePool m_pool;
    TopologyCache           m_topo_cache;
};

} // namespace Ignis
