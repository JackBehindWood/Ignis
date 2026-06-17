#pragma once

#include "ShaderReflection.h"
#include "Ignis/Foundation/Vector.h"

namespace Ignis
{

static constexpr uint8_t kStageBit_Vertex = 0x01u;
static constexpr uint8_t kStageBit_Pixel  = 0x02u;

enum class MergedResourceType : uint8_t
{
    UniformBuffer   = 0,
    StorageBuffer   = 1,
    StorageTexture  = 2,
    SeparateImage   = 3,
    SeparateSampler = 4,
};

struct MergedResourceBinding
{
    String             name;
    uint32_t           set;
    uint32_t           binding;
    MergedResourceType type;
    bool               is_unbounded = false;
    uint8_t            stage_mask   = 0;
};

struct MergedPipelineReflection
{
    Vector<MergedResourceBinding> bindings;

    static MergedPipelineReflection merge(const ShaderReflection* vs, const ShaderReflection* ps);

    Vector<uint32_t> unique_sets() const;
};

} // namespace Ignis
