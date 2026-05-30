#include "edpch.h"
#include "LayoutStorage.h"

namespace Ignis
{

bool LayoutStorage::save(WorkspaceId, const String&, const LayoutNode&)
{
    return false;
}

bool LayoutStorage::load(WorkspaceId, const String&, LayoutNode&)
{
    return false;
}

bool LayoutStorage::migrate(uint32_t, uint32_t, LayoutNode&)
{
    return true;
}

} // namespace Ignis
