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
    String   name; // HLSL semantic (e.g. "NORMAL", "TEXCOORD0"), not the DXC-mangled variable name
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
    Vector<SpirvStageInput>   stage_outputs;
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

    inline Vector<uint32_t> compile_to_binary(const String& source, const char* entry_point, GRIShaderStage stage,
                                              const Vector<Pair<String, String>>& defines = {})
    {
        return compile_to_target(source, entry_point, to_execution_model(stage), defines);
    }
    inline String compile_from_binary(const uint32_t* spirv, uint32_t word_count, GRIShaderStage stage)
    {
        return compile_from_target(spirv, word_count, to_execution_model(stage));
    }
    inline Vector<uint8_t> spirv_to_backend_binary(const uint32_t* spirv, uint32_t word_count, GRIShaderStage stage)
    {
        return compile_to_backend(spirv, word_count, to_execution_model(stage));
    }
    inline Vector<uint32_t> backend_binary_to_spirv(const uint8_t* data, uint32_t size)
    {
        return compile_from_backend(data, size);
    }

    // Link-aware PS compilation: remaps PS input Location decorations to match VS outputs before
    // backend code generation. Default falls back to an unlinked compile of the PS stage.
    virtual Vector<uint8_t> spirv_to_backend_binary_linked(const uint32_t* vs_spirv, uint32_t vs_word_count,
                                                           const uint32_t* ps_spirv, uint32_t ps_word_count)
    {
        return compile_to_backend(ps_spirv, ps_word_count, spv::ExecutionModelFragment);
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
