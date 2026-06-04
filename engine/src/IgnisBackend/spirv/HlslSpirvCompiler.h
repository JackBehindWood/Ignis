#pragma once
#include "SpirvCompiler.h"

namespace Ignis
{
class HlslSpirvCompiler : public SpirvCompiler
{
public:
    // Register a virtual include file served from an in-memory source string.
    // virtual_path must match the path as written in the #include directive,
    // e.g. "Ignis/Core.hlsl" to satisfy #include <Ignis/Core.hlsl>.
    void register_virtual_include(const String& virtual_path, const String& source);

    // Set the base directory used to resolve relative #include directives.
    // Should be the directory containing the HLSL source file being compiled.
    void set_source_directory(const Path& dir);

protected:
    Vector<uint32_t> compile_to_target(const String& source, const char* entry_point, spv::ExecutionModel exec_model,
                                       const Vector<Pair<String, String>>& defines) override;
    String           compile_from_target(const uint32_t*, uint32_t, spv::ExecutionModel) override
    {
        return {};
    }

private:
    UnorderedMap<String, String> m_virtual_includes;
    Path                         m_source_dir;
};

} // namespace Ignis
