#pragma once
#include "SpirvCompiler.h"

namespace Ignis
{

struct MslCompileOptions
{
    enum class Platform : uint8_t
    {
        macOS,
        iOS
    };

    Platform platform    = Platform::macOS;
    uint32_t msl_version = 20100; // MSL 2.1
};

class MslSpirvCompiler : public SpirvCompiler
{
public:
    MslSpirvCompiler() = default;
    explicit MslSpirvCompiler(MslCompileOptions options)
        : m_options(options)
    {
    }

    // Link-aware PS backend compilation. Reflects VS outputs and remaps PS input Location
    // decorations to match before MSL generation, preventing DCE-induced interface mismatches.
    Vector<uint8_t> spirv_to_backend_binary_linked(const uint32_t* vs_spirv, uint32_t vs_word_count,
                                                   const uint32_t* ps_spirv, uint32_t ps_word_count) override;

protected:
    Vector<uint32_t> compile_to_target(const String&, const char*, spv::ExecutionModel,
                                       const Vector<Pair<String, String>>&) override
    {
        return {};
    }
    String compile_from_target(const uint32_t* spirv, uint32_t word_count, spv::ExecutionModel exec_model) override;
    Vector<uint8_t> compile_to_backend(const uint32_t* spirv, uint32_t word_count,
                                       spv::ExecutionModel exec_model) override;

private:
    Vector<uint8_t> msl_to_metallib(const String& msl) const;

    MslCompileOptions m_options;
};

} // namespace Ignis
