#pragma once

#include "SpirvCompiler.h"

namespace Ignis
{

struct MslCompileOptions
{
    enum class Platform : uint8_t { macOS, iOS };

    Platform platform    = Platform::macOS;
    uint32_t msl_version = 20100; // MSL 2.1 — build with CompilerMSL::Options::make_msl_version()
};

// SPIR-V → MSL cross-compiler using SPIRV-Cross.
class MslSpirvCompiler : public SpirvCompiler
{
public:
    MslSpirvCompiler() = default;
    explicit MslSpirvCompiler(MslCompileOptions options) : m_options(options) {}

protected:
    String compile_native(const uint32_t* spirv, uint32_t word_count, spv::ExecutionModel exec_model) override;

private:
    MslCompileOptions m_options;
};

} // namespace Ignis
