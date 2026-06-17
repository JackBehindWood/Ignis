#include "igpch.h"
#include "MergedPipelineReflection.h"

namespace Ignis
{

static uint64_t binding_key(uint32_t set, uint32_t binding, MergedResourceType type)
{
    return (static_cast<uint64_t>(type) << 48) | (static_cast<uint64_t>(set) << 16) | static_cast<uint64_t>(binding);
}

static const char* type_name(MergedResourceType t)
{
    switch (t)
    {
        case MergedResourceType::UniformBuffer:
            return "UniformBuffer";
        case MergedResourceType::StorageBuffer:
            return "StorageBuffer";
        case MergedResourceType::StorageTexture:
            return "StorageTexture";
        case MergedResourceType::SeparateImage:
            return "SeparateImage";
        case MergedResourceType::SeparateSampler:
            return "SeparateSampler";
    }
    return "Unknown";
}

MergedPipelineReflection MergedPipelineReflection::merge(const ShaderReflection* vs, const ShaderReflection* ps)
{
    MergedPipelineReflection       result;
    UnorderedMap<uint64_t, size_t> index;

    struct TypedSlice
    {
        const Vector<ShaderResourceBinding>* vec;
        MergedResourceType                   type;
    };

    auto process_stage = [&](const ShaderReflection* refl, uint8_t stage_bit)
    {
        if (!refl)
        {
            return;
        }

        const TypedSlice slices[] = {
            {&refl->uniform_buffers, MergedResourceType::UniformBuffer},
            {&refl->storage_buffers, MergedResourceType::StorageBuffer},
            {&refl->storage_textures, MergedResourceType::StorageTexture},
            {&refl->separate_images, MergedResourceType::SeparateImage},
            {&refl->separate_samplers, MergedResourceType::SeparateSampler},
        };

        for (const TypedSlice& slice : slices)
        {
            for (const ShaderResourceBinding& b : *slice.vec)
            {
                const uint64_t key = binding_key(b.set, b.binding, slice.type);
                auto           it  = index.find(key);

                if (it == index.end())
                {
                    index.emplace(key, result.bindings.size());
                    result.bindings.push_back({b.name, b.set, b.binding, slice.type, b.is_unbounded, stage_bit});
                }
                else
                {
                    MergedResourceBinding& existing = result.bindings[it->second];

                    if (existing.is_unbounded != b.is_unbounded)
                    {
                        IG_CORE_CRITICAL("MergedPipelineReflection: is_unbounded mismatch — "
                                         "{} '{}' / '{}' at set={} binding={}: "
                                         "one stage declares unbounded, the other does not",
                                         type_name(slice.type), existing.name, b.name, b.set, b.binding);
                        IG_CORE_ASSERT(false, "Cross-stage is_unbounded mismatch");
                    }

                    existing.stage_mask = static_cast<uint8_t>(existing.stage_mask | stage_bit);
                }
            }
        }
    };

    process_stage(vs, kStageBit_Vertex);
    process_stage(ps, kStageBit_Pixel);

    return result;
}

Vector<uint32_t> MergedPipelineReflection::unique_sets() const
{
    Vector<uint32_t> sets;
    for (const MergedResourceBinding& b : bindings)
    {
        bool found = false;
        for (uint32_t s : sets)
        {
            if (s == b.set)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            sets.push_back(b.set);
        }
    }
    std::sort(sets.begin(), sets.end());
    return sets;
}

} // namespace Ignis
