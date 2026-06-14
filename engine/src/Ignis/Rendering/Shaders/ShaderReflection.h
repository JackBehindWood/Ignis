#pragma once

#include "Ignis/Foundation/String.h"
#include "Ignis/Foundation/Vector.h"

namespace Ignis
{

struct ShaderResourceBinding
{
    String   name;
    uint32_t set;
    uint32_t binding;
    bool     is_unbounded = false; // true for OpTypeRuntimeArray (bindless unbounded arrays)
};

struct ShaderStageInput
{
    String   name;
    uint32_t location;
};

struct ShaderPushConstant
{
    String   name;
    uint32_t size;
};

struct ShaderReflection
{
    Vector<ShaderResourceBinding> uniform_buffers;
    Vector<ShaderResourceBinding> storage_buffers;
    Vector<ShaderResourceBinding> storage_textures;
    Vector<ShaderResourceBinding> separate_images;
    Vector<ShaderResourceBinding> separate_samplers;
    Vector<ShaderStageInput>      stage_inputs;
    Vector<ShaderStageInput>      stage_outputs;
    Vector<ShaderPushConstant>    push_constants;
    uint32_t                      threadgroup_size_x = 0;
    uint32_t                      threadgroup_size_y = 0;
    uint32_t                      threadgroup_size_z = 0;
};

} // namespace Ignis
