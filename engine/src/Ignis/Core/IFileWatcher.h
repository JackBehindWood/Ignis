#pragma once

#include "Ignis/Foundation/UniquePtr.h"
#include "Ignis/Foundation/Filesystem.h"

namespace Ignis
{
class AssetManager; // forward — avoid pulling Asset layer into Core header

class IFileWatcher
{
public:
    virtual ~IFileWatcher() = default;

    // Platform factory — implemented in IgnisBackend.
    static UniquePtr<IFileWatcher> create();

    // Register a source file for change notifications. id is the raw UUID value.
    virtual void watch(const Path& source_path, uint64_t id) = 0;
    virtual void unwatch(uint64_t id)                        = 0;

    // Non-blocking: drain any pending OS events and dispatch flag_for_reload calls.
    // Must be called from the main thread.
    virtual void consume_events(AssetManager& manager) = 0;
};

} // namespace Ignis
