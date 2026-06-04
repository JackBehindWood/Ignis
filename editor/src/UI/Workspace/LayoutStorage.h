#pragma once

#include "WorkspaceDefinition.h"

namespace Ignis
{

class LayoutStorage
{
public:
    bool save(WorkspaceId id, const String& path, const LayoutNode& layout);
    bool load(WorkspaceId id, const String& path, LayoutNode& out);

    // Applies in-place migrations from from_version up to to_version.
    bool migrate(uint32_t from_version, uint32_t to_version, LayoutNode& layout);
};

} // namespace Ignis
