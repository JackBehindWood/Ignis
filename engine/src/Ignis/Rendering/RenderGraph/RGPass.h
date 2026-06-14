#pragma once

#include <Ignis/Foundation/Vector.h>
#include <Ignis/Rendering/GRI/GRIDefinitions.h>
#include "RGResource.h"

namespace Ignis
{

class GRICommandList;

enum class RGPassType
{
    Graphics,
    Compute
};

struct RGPassBase
{
    RGPassType  pass_type       = RGPassType::Graphics;
    const char* name            = nullptr;
    bool        is_culled       = false;
    uint32_t    num_color_slots = 0;
    bool        has_depth       = false;
    bool        depth_read_only = false; // Load-only; does not claim writer ownership

    Vector<uint16_t> texture_reads;
    Vector<uint16_t> buffer_reads;
    Vector<uint16_t> buffer_writes;
    Vector<uint16_t> texture_writes;

    Vector<uint16_t> storage_buffer_reads;
    Vector<uint16_t> storage_buffer_writes;
    Vector<uint16_t> storage_texture_reads;
    Vector<uint16_t> storage_texture_writes;

    RGInternal::AttachmentSlot color_slots[max_simultaneous_render_targets];
    RGInternal::AttachmentSlot depth_slot = {};

    virtual void run_execute(GRICommandList& cmd) = 0;
    virtual void run_destructor()                 = 0;

protected:
    ~RGPassBase() = default;
};

template <typename ExecuteFn>
struct TypedRGPass final : public RGPassBase
{
    ExecuteFn execute_fn;

    explicit TypedRGPass(ExecuteFn&& fn)
        : execute_fn(std::forward<ExecuteFn>(fn))
    {
    }

    void run_execute(GRICommandList& cmd) override
    {
        execute_fn(cmd);
    }

    void run_destructor() override
    {
        this->~TypedRGPass();
    }
};

} // namespace Ignis
