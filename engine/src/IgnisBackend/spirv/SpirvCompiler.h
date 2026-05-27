#pragma once

#include "Ignis/Rendering/Shaders/ShaderTarget.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"

#include <spirv.hpp>

namespace Ignis
{
struct SpirvBindingInfo
{
    String   name;
    uint32_t set;
    uint32_t binding;
};

struct SpirvStageInput
{
    String   name;
    uint32_t location;
};

struct SpirvPushConstant
{
    String   name;
    uint32_t size;
};

struct SpirvReflection
{
    Vector<SpirvBindingInfo>  uniform_buffers;
    Vector<SpirvBindingInfo>  storage_buffers;
    Vector<SpirvBindingInfo>  separate_images;
    Vector<SpirvBindingInfo>  separate_samplers;
    Vector<SpirvStageInput>   stage_inputs;
    Vector<SpirvPushConstant> push_constants;
};

class SpirvCompiler
{
private:
    spv::ExecutionModel to_execution_model(GRIShaderStage stage)
    {
        switch (stage)
        {
            case GRIShaderStage::Vertex:
                return spv::ExecutionModelVertex;
            case GRIShaderStage::Pixel:
                return spv::ExecutionModelFragment;
            case GRIShaderStage::Compute:
                return spv::ExecutionModelGLCompute;
            default:
                IG_CORE_ASSERT(false, "SpirvCompiler: unsupported shader stage");
                return spv::ExecutionModelVertex;
        }
    }

public:
    virtual ~SpirvCompiler() = default;

    // HLSL source → SPIR-V words
    inline Vector<uint32_t> compile_to_binary(const String& source, const char* entry_point, GRIShaderStage stage,
                                              const Vector<Pair<String, String>>& defines = {})
    {
        return compile_to_target(source, entry_point, to_execution_model(stage), defines);
    }
    // SPIR-V words → native source text (e.g. MSL)
    inline String compile_from_binary(const uint32_t* spirv, uint32_t word_count, GRIShaderStage stage)
    {
        return compile_from_target(spirv, word_count, to_execution_model(stage));
    }
    // SPIR-V words → compiled backend binary (e.g. .metallib bytes, raw SPIR-V bytes for Vulkan)
    inline Vector<uint8_t> spirv_to_backend_binary(const uint32_t* spirv, uint32_t word_count, GRIShaderStage stage)
    {
        return compile_to_backend(spirv, word_count, to_execution_model(stage));
    }
    // Compiled backend binary → SPIR-V words (identity for Vulkan; {} for irreversible targets like Metal)
    inline Vector<uint32_t> backend_binary_to_spirv(const uint8_t* data, uint32_t size)
    {
        return compile_from_backend(data, size);
    }

    SpirvReflection reflect(const uint32_t* spirv, uint32_t word_count);

    static UniquePtr<SpirvCompiler> create(ShaderTarget target);

protected:
    virtual Vector<uint32_t> compile_to_target(const String& source, const char* entry_point,
                                               spv::ExecutionModel                 exec_model,
                                               const Vector<Pair<String, String>>& defines)                        = 0;
    virtual String compile_from_target(const uint32_t* spirv, uint32_t word_count, spv::ExecutionModel exec_model) = 0;
    virtual Vector<uint8_t> compile_to_backend(const uint32_t* spirv, uint32_t word_count,
                                               spv::ExecutionModel exec_model)
    {
        return {};
    }
    virtual Vector<uint32_t> compile_from_backend(const uint8_t* data, uint32_t size)
    {
        return {};
    }
};

} // namespace Ignis