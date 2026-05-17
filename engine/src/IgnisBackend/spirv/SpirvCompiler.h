#pragma once

#include "Ignis/Rendering/GRI/GRIDefinitions.h"

#include <spirv.hpp>

namespace Ignis
{

    // A resource bound to a descriptor set slot (uniform buffer, texture, sampler, etc.)
    struct SpirvBindingInfo
    {
        String   name;
        uint32_t set;
        uint32_t binding;
    };

    // A vertex/fragment stage input variable (e.g. HLSL semantic → SPIR-V Location).
    struct SpirvStageInput
    {
        String   name;
        uint32_t location;
    };

    // A push-constant block and its total byte size.
    struct SpirvPushConstant
    {
        String   name;
        uint32_t size; // bytes
    };

    // All resource metadata extracted from one SPIR-V module.
    // Populated by SpirvCompiler::reflect() — backend-agnostic SPIR-V binding numbers.
    struct SpirvReflection
    {
        Vector<SpirvBindingInfo>  uniform_buffers;   // cbuffer / UBO
        Vector<SpirvBindingInfo>  storage_buffers;   // RWBuffer / SSBO
        Vector<SpirvBindingInfo>  separate_images;   // Texture2D / t-registers
        Vector<SpirvBindingInfo>  separate_samplers; // SamplerState / s-registers
        Vector<SpirvStageInput>   stage_inputs;      // vertex attributes / frag inputs
        Vector<SpirvPushConstant> push_constants;
    };

    // Non-virtual interface for SPIR-V cross-compilation and reflection.
    // Subclasses implement compile_native() for their target language (MSL, GLSL, etc.).
    class SpirvCompiler
    {
    public:
        virtual ~SpirvCompiler() = default;

        String compile(const uint32_t* spirv, uint32_t word_count, GRIShaderStage stage);
        SpirvReflection reflect(const uint32_t* spirv, uint32_t word_count);

    protected:
        // Backend-specific translation. exec_model is resolved from GRIShaderStage by compile().
        virtual String compile_native(const uint32_t* spirv, uint32_t word_count, spv::ExecutionModel exec_model) = 0;
    };

} // namespace Ignis
