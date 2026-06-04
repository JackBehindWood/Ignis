#pragma once

#include <Ignis/Foundation/Foundation.h>
#include "IAssetEventObserver.h"

namespace Ignis
{

class AssetDirectoryModel : public IAssetEventObserver
{
public:
    struct Entry
    {
        Path      path;
        bool      is_dir;
        String    display_name;
        String    type_label;
        uintmax_t file_size      = 0;
        bool      status_missing = false;
    };

    void init(const Path& assets_root);
    void shutdown();
    void tick(float ts); // mtime poll + deferred refresh flush

    void navigate_to(const Path& dir);
    void navigate_back();
    void navigate_forward();
    void refresh();

    const Path& current_dir() const
    {
        return m_current_dir;
    }
    const Path& assets_root() const
    {
        return m_root;
    }
    const Vector<Entry>& entries() const
    {
        return m_entries;
    }
    bool can_go_back() const
    {
        return !m_history_back.empty();
    }
    bool can_go_forward() const
    {
        return !m_history_forward.empty();
    }

    void        set_search(const char* text);
    void        set_filter_type(int32_t type);
    void        set_filter_tag(const char* tag);
    const char* search_buf() const
    {
        return m_search_buf;
    }
    int32_t filter_type() const
    {
        return m_filter_type;
    }
    const char* filter_tag() const
    {
        return m_filter_tag;
    }

    char* search_buf_mutable()
    {
        return m_search_buf;
    }
    char* filter_tag_mutable()
    {
        return m_filter_tag;
    }

    // IAssetEventObserver
    void on_asset_imported(const Path&) override
    {
        m_needs_refresh = true;
    }
    void on_asset_renamed(const Path&, const Path&) override
    {
        m_needs_refresh = true;
    }
    void on_asset_deleted(const Path&) override
    {
        m_needs_refresh = true;
    }
    void on_directory_changed() override
    {
        m_needs_refresh = true;
    }

private:
    void navigate_raw(const Path& dir);
    void rebuild_entries();

    Path          m_root;
    Path          m_current_dir;
    Vector<Entry> m_entries;
    Deque<Path>   m_history_back;
    Deque<Path>   m_history_forward;

    char    m_search_buf[256] = {};
    int32_t m_filter_type     = 0;
    char    m_filter_tag[64]  = {};

    RelaxedAtomic<bool>        m_needs_refresh{false};
    float                      m_poll_timer = 0.0f;
    Filesystem::file_time_type m_dir_mtime  = {};
};

} // namespace Ignis
