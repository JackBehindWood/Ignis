#include "igpch.h"
#include "MacOSFileWatcher.h"

#ifdef IG_PLATFORM_MACOS

#include "Ignis/Asset/AssetManager.h"
#include "Ignis/Asset/Asset.h"

namespace Ignis
{

UniquePtr<IFileWatcher> IFileWatcher::create()
{
    return create_unique<MacOSFileWatcher>();
}

MacOSFileWatcher::~MacOSFileWatcher()
{
    stop_stream();
}

void MacOSFileWatcher::watch(const Path& source_path, uint64_t id)
{
    const uint64_t h = RuntimePathId::fnv1a(source_path.string());

    if (m_path_map.count(h))
    {
        return;
    }

    m_path_map[h]    = id;
    m_id_to_hash[id] = h;

    const String dir = Filesystem::canonical(source_path).parent_path().string();
    if (m_watched_dirs.insert(dir).second)
    {
        restart_stream();
    }
}

void MacOSFileWatcher::unwatch(uint64_t id)
{
    auto it = m_id_to_hash.find(id);
    if (it == m_id_to_hash.end())
    {
        return;
    }

    const uint64_t h = it->second;
    m_path_map.erase(h);
    m_id_to_hash.erase(it);
}

void MacOSFileWatcher::consume_events(AssetManager& manager)
{
    // m_pending is populated by s_callback, which fires on the main dispatch queue
    // during glfwPollEvents (the frame's run-loop drain) — no explicit pump needed here.
    for (uint64_t raw_id : m_pending)
    {
        manager.flag_for_reload(AssetID(raw_id));
    }
    m_pending.clear();
}

void MacOSFileWatcher::restart_stream()
{
    stop_stream();

    if (m_watched_dirs.empty())
    {
        return;
    }

    CFMutableArrayRef paths = CFArrayCreateMutable(nullptr, 0, &kCFTypeArrayCallBacks);
    for (const String& dir : m_watched_dirs)
    {
        CFStringRef s = CFStringCreateWithCString(nullptr, dir.c_str(), kCFStringEncodingUTF8);
        CFArrayAppendValue(paths, s);
        CFRelease(s);
    }

    FSEventStreamContext ctx = {0, this, nullptr, nullptr, nullptr};

    m_stream = FSEventStreamCreate(nullptr, &MacOSFileWatcher::s_callback, &ctx, paths, kFSEventStreamEventIdSinceNow,
                                   0.25, // latency in seconds — balances responsiveness vs. event batching
                                   kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagNoDefer);

    CFRelease(paths);

    // Use the main dispatch queue so events are delivered on the main thread during
    // the frame's event pump (glfwPollEvents / CFRunLoop drain) — no manual run-loop
    // pumping required and avoids the deprecated RunLoop scheduling API.
    FSEventStreamSetDispatchQueue(m_stream, dispatch_get_main_queue());
    FSEventStreamStart(m_stream);
}

void MacOSFileWatcher::stop_stream()
{
    if (!m_stream)
    {
        return;
    }
    FSEventStreamStop(m_stream);
    FSEventStreamInvalidate(m_stream);
    FSEventStreamRelease(m_stream);
    m_stream = nullptr;
}

void MacOSFileWatcher::s_callback(ConstFSEventStreamRef /*stream*/, void* client_info, size_t num_events,
                                  void* event_paths, const FSEventStreamEventFlags* event_flags,
                                  const FSEventStreamEventId* /*event_ids*/)
{
    MacOSFileWatcher* self  = static_cast<MacOSFileWatcher*>(client_info);
    const char**      paths = static_cast<const char**>(event_paths);

    for (size_t i = 0; i < num_events; ++i)
    {
        const FSEventStreamEventFlags f = event_flags[i];

        // Only care about actual file content changes or renames
        constexpr FSEventStreamEventFlags k_care =
            kFSEventStreamEventFlagItemModified | kFSEventStreamEventFlagItemCreated |
            kFSEventStreamEventFlagItemRenamed | kFSEventStreamEventFlagItemInodeMetaMod;

        if (!(f & k_care))
        {
            continue;
        }
        if (f & kFSEventStreamEventFlagItemIsDir)
        {
            continue;
        }

        const uint64_t h  = RuntimePathId::fnv1a(paths[i]);
        auto           it = self->m_path_map.find(h);
        if (it != self->m_path_map.end())
        {
            self->m_pending.push_back(it->second);
        }
    }
}

} // namespace Ignis

#endif // IG_PLATFORM_MACOS
