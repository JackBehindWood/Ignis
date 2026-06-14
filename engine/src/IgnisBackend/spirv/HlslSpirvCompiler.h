#pragma once
#include "SpirvCompiler.h"

namespace Ignis
{
class HlslSpirvCompiler : public SpirvCompiler
{
public:
    // Register a virtual include file served from an in-memory source string.
    // virtual_path must match the path as written in the #include directive,
    // e.g. "Ignis.hlsl" to satisfy #include <Ignis.hlsl>.
    void register_virtual_include(const String& virtual_path, const String& source);

    // Set the ordered list of directories searched for relative #include resolution.
    // First match wins. Replaces any previously set directories.
    void set_include_dirs(const Vector<Path>& dirs);

protected:
    Vector<uint32_t> compile_to_target(const String& source, const char* entry_point, spv::ExecutionModel exec_model,
                                       const Vector<Pair<String, String>>& defines) override;
    String           compile_from_target(const uint32_t*, uint32_t, spv::ExecutionModel) override
    {
        return {};
    }

private:
    UnorderedMap<String, String> m_virtual_includes;
    Vector<Path>                 m_include_dirs;
};

} // namespace Ignis
