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
    Vector<Path>                 include_dirs; // searched in order; first match wins
};

class ShaderCompiler
{
public:
    explicit ShaderCompiler(ShaderTarget target);
    ~ShaderCompiler();

    Vector<ShaderStageOutput> compile(const String& hlsl_source, const ShaderCompilerOptions& opts = {});

    // Register a virtual include file served from memory.
    // virtual_path must match the path written in the #include directive verbatim
    // (e.g. "Ignis.hlsl" for #include <Ignis.hlsl>).
    void register_virtual_include(const String& virtual_path, const String& source);

    // Set the ordered list of directories used to resolve relative #include directives.
    // Applied for all subsequent compile() calls. opts.include_dirs takes precedence
    // if non-empty at call time.
    void set_include_dirs(const Vector<Path>& dirs);

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
