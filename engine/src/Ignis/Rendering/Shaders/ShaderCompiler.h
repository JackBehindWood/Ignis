#pragma once

#include "ShaderTarget.h"
#include "ShaderReflection.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"

namespace Ignis
{
class SpirvCompiler;
class HlslSpirvCompiler;

struct ShaderStageOutput
{
    GRIShaderStage   stage;
    String           entry_point;
    Vector<uint8_t>  bytecode;
    ShaderReflection reflection;
};

struct ShaderCompilerOptions
{
    struct StageEntry
    {
        GRIShaderStage stage;
        String         entry_point = "";
    };

    StageEntry                   stages[(size_t)GRIShaderStage::COUNT];
    uint8_t                      count = 0;
    Vector<Pair<String, String>> defines;
};

class ShaderCompiler
{
public:
    explicit ShaderCompiler(ShaderTarget target);
    ~ShaderCompiler();

    Vector<ShaderStageOutput> compile(const String& hlsl_source, const ShaderCompilerOptions& opts = {});

    // Register a virtual include file served from memory.
    // virtual_path must match the path written in the #include directive verbatim
    // (e.g. "Ignis/Core.hlsl" for #include <Ignis/Core.hlsl>).
    void register_virtual_include(const String& virtual_path, const String& source);

    // Set the directory used to resolve relative #include directives for the next compile call.
    void set_source_directory(const Path& dir);

    ShaderTarget get_target() const
    {
        return m_target;
    }

private:
    ShaderTarget             m_target;
    UniquePtr<SpirvCompiler> m_hlsl;
    UniquePtr<SpirvCompiler> m_backend;
};

} // namespace Ignis
