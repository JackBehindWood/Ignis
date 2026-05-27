#pragma once
#include "SpirvCompiler.h"

namespace Ignis
{
class HlslSpirvCompiler : public SpirvCompiler
{
protected:
    Vector<uint32_t> compile_to_target(const String& source, const char* entry_point, spv::ExecutionModel exec_model,
                                       const Vector<Pair<String, String>>& defines) override;
    String           compile_from_target(const uint32_t*, uint32_t, spv::ExecutionModel) override
    {
        return {};
    }
};

} // namespace Ignis