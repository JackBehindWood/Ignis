#pragma once

#include "RGResource.h"
#include "RGPass.h"
#include "RenderGraph.h"
#include <Ignis/Foundation/Vector.h>
#include <Ignis/Foundation/TypeTraits.h>
#include <Ignis/Rendering/GRI/GRIResource.h>

namespace Ignis
{

class GRI;
class GRICommandList;

class RGBuilder
{
public:
    RGBuilder();
    ~RGBuilder() = default;

    RGBuilder(const RGBuilder&) = delete;
    RGBuilder& operator=(const RGBuilder&) = delete;

    // --- Resource Declaration ---
    RGTextureHandle create_texture(const char* name, const RGTextureDesc& desc);
    RGTextureHandle import_backbuffer();
    RGTextureHandle import_texture(const char* name, GRITexture2D* physical);

    RGBufferHandle  create_buffer(const char* name, const RGBufferDesc& desc);
    RGBufferHandle  import_buffer(const char* name, GRIBuffer* physical);

    // --- Stateful Dependency Bindings ---
    void read_texture(RGTextureHandle h);
    void write_render_target(uint32_t slot, RGTextureHandle h, const RGColorAttachmentDesc& desc = {});
    void write_depth_stencil(RGTextureHandle h, const RGDepthAttachmentDesc& desc = {});
    void write_storage_texture(RGTextureHandle h);

    void read_buffer(RGBufferHandle h);
    void write_buffer(RGBufferHandle h);

    // --- Arena Parameter Allocation ---
    // T must be trivially destructible; arena is soft-reset without calling destructors.
    template<typename T>
    T* alloc_params()
    {
        static_assert(IsTriviallyDestructible<T>,
            "alloc_params<T>: T must be trivially destructible (arena-allocated, no destructor called)");
        void* mem = arena_alloc(sizeof(T), alignof(T));
        return new (mem) T{};
    }

    // --- Single Execute Lambda Registration ---
    template<typename ExecuteFn>
    void add_pass(const char* name, ExecuteFn&& execute)
    {
        using PassType = TypedRGPass<Decay<ExecuteFn>>;
        void* mem  = arena_alloc(sizeof(PassType), alignof(PassType));
        auto* pass = new (mem) PassType(std::forward<ExecuteFn>(execute));
        commit_pass(name, pass);
    }

    // --- Params-Style Pass Registration ---
    // execute signature: void(T*, GRICommandList&)
    template<typename T, typename ExecuteFn>
    void add_pass(const char* name, T* params, ExecuteFn&& execute)
    {
        add_pass(name, [params, fn = std::forward<ExecuteFn>(execute)](GRICommandList& cmd) {
            fn(params, cmd);
        });
    }

    // --- Physical Resource Access (valid during execute) ---
    GRITexture2D* get_physical(RGTextureHandle h) const;
    GRIBuffer*    get_physical(RGBufferHandle h)  const;

    // --- Frame Pipeline Entry Point ---
    void execute(GRICommandList& cmd);

private:
    void* arena_alloc(size_t size, size_t alignment);
    void  commit_pass(const char* name, RGPassBase* pass);

    RenderGraph m_graph;

    struct PassDependencies
    {
        Vector<uint16_t>           texture_reads;
        Vector<uint16_t>           buffer_reads;
        Vector<uint16_t>           texture_writes;
        Vector<uint16_t>           buffer_writes;
        RGInternal::AttachmentSlot color_slots[max_simultaneous_render_targets];
        RGInternal::AttachmentSlot depth_slot    = {};
        uint32_t                   num_color_slots = 0;
        bool                       has_depth       = false;

        void reset();
    } m_current_deps;
};

} // namespace Ignis
