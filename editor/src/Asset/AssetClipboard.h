#pragma once

#include <Ignis/Foundation/Foundation.h>

namespace Ignis
{

class CommandDispatcher;
class AssetDirectoryModel;

class AssetClipboard
{
public:
    void copy(const Vector<Path>& paths);
    void cut(const Vector<Path>& paths); // records intent; no filesystem op at cut time

    // Call once per frame from the panel's draw(). Handles conflict detection and
    // dispatches PasteAssetsCmd through dispatcher (may be null → direct filesystem op).
    void paste_into(const Path& dest, CommandDispatcher* dispatcher, AssetDirectoryModel* model);

    // Must be called once per frame from the panel's draw() to render the overwrite confirm modal.
    void draw_confirm_modal(CommandDispatcher* dispatcher, AssetDirectoryModel* model);

    bool has_content() const
    {
        return !m_clipboard.empty();
    }
    bool is_cut() const
    {
        return m_is_cut;
    }
    bool needs_confirm_open() const
    {
        return m_open_confirm;
    }
    const Vector<Path>& paths() const
    {
        return m_clipboard;
    }
    void clear()
    {
        m_clipboard.clear();
        m_is_cut = false;
    }

private:
    void execute_paste(const Vector<Path>& srcs, const Path& dest, bool is_cut, bool overwrite,
                       CommandDispatcher* dispatcher, AssetDirectoryModel* model);

    Vector<Path> m_clipboard;
    bool         m_is_cut = false;

    Vector<Path> m_pending_srcs;
    Path         m_pending_dest;
    bool         m_pending_is_cut = false;
    bool         m_open_confirm   = false;
};

} // namespace Ignis
