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
    uint32_t msl_version = 30000; // MSL 3.0 (Metal 3, macOS 13+)
};

class MslSpirvCompiler : public SpirvCompiler
{
public:
    MslSpirvCompiler() = default;
    explicit MslSpirvCompiler(MslCompileOptions options)
        : m_options(options)
    {
    }

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
