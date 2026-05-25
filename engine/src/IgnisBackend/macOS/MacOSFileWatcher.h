#pragma once

#ifdef IG_PLATFORM_MACOS

#include "Ignis/Core/IFileWatcher.h"
#include "Ignis/Foundation/Vector.h"
#include "Ignis/Foundation/UnorderedMap.h"
#include "Ignis/Foundation/UnorderedSet.h"
#include "Ignis/Foundation/String.h"
#include "Ignis/Foundation/RuntimePathId.h"

#include <CoreServices/CoreServices.h>
#include <dispatch/dispatch.h>

namespace Ignis
{
class MacOSFileWatcher final : public IFileWatcher
{
public:
    MacOSFileWatcher() = default;
    ~MacOSFileWatcher() override;

    void watch(const Path& source_path, uint64_t id) override;
    void unwatch(uint64_t id) override;
    void consume_events(AssetManager& manager) override;

private:
    static void s_callback(ConstFSEventStreamRef stream, void* client_info, size_t num_events, void* event_paths,
                           const FSEventStreamEventFlags* event_flags, const FSEventStreamEventId* event_ids);

    void restart_stream();
    void stop_stream();

    FSEventStreamRef m_stream = nullptr;

    UnorderedMap<uint64_t, uint64_t> m_path_map;
    Vector<uint64_t>                 m_pending;
    UnorderedSet<String>             m_watched_dirs;
    UnorderedMap<uint64_t, uint64_t> m_id_to_hash;
};

} // namespace Ignis

#endif // IG_PLATFORM_MACOS
