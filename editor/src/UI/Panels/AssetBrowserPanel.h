#pragma once

#include "IPanel.h"
#include <Ignis/Input/InputContext.h>
#include "Asset/AssetDirectoryModel.h"
#include "Asset/AssetClipboard.h"

namespace Ignis
{

class CommandDispatcher;

class AssetBrowserPanel : public IPanel
{
public:
    AssetBrowserPanel();
    ~AssetBrowserPanel();

    PanelId     get_id() const override;
    const char* get_title() const override
    {
        return "Asset Browser";
    }
    void update(float ts, IWorkspaceData* ctx) override;
    void draw(IWorkspaceData* ctx) override;

    virtual ImGuiWindowFlags get_window_flags() const override
    {
        return ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    }

private:
    void handle_keyboard_nav(const Vector<int32_t>& filtered);
    void draw_directory_tree();
    void draw_directory_tree_node(const Path& dir);
    void draw_breadcrumb();
    void draw_favorites_bar();
    void draw_toolbar();
    void draw_grid_view(const Vector<int32_t>& filtered);
    void draw_list_view(const Vector<int32_t>& filtered);
    void draw_entry(int32_t fi, const Vector<int32_t>& filtered, const AssetDirectoryModel::Entry& e);
    void draw_entry_context_menu(const AssetDirectoryModel::Entry& e);
    void draw_background_context_menu();
    void draw_add_tag_modal();
    void draw_dependency_popup();
    void draw_delete_confirm_modal();
    void commit_rename();
    void draw_hover_tooltip(const AssetDirectoryModel::Entry& e) const;
    void draw_asset_status_badge(const AssetDirectoryModel::Entry& e) const;
    void drop_into_dir(const AssetDragPayload* p, const Path& dest, bool is_copy);
    void draw_background_drop_target();

    bool is_selected(const Path& p) const;
    void select_single(int32_t fi, const Vector<int32_t>& filtered);
    void select_toggle(const Path& p);
    void select_range(int32_t from_fi, int32_t to_fi, const Vector<int32_t>& filtered);

    // Prunes m_selected_paths entries that no longer exist in the current model entries.
    void sync_selection();

    void push_recent(const Path& p);

    // ---- Extracted sub-systems ----
    AssetDirectoryModel m_model;
    AssetClipboard      m_clipboard;

    // ---- UI-local state ----
    InputContext       m_input_ctx;
    CommandDispatcher* m_dispatcher = nullptr;

    Path m_renaming_path;
    char m_rename_buf[256] = {};
    bool m_is_grid_view    = true;

    Vector<Path> m_selected_paths;
    int32_t      m_last_clicked_idx = -1;

    Vector<Path>                         m_favorites;
    Deque<Path>                          m_recent_files;
    UnorderedMap<String, Vector<String>> m_asset_tags;

    Path m_tag_target;
    char m_tag_buf[64]  = {};
    bool m_open_add_tag = false;

    bool m_open_dep_popup = false;
    Path m_dep_popup_target;

    bool m_open_delete_confirm = false;

    bool m_col_show_type  = true;
    bool m_col_show_size  = true;
    bool m_col_show_dirty = false;
    bool m_col_show_tags  = false;

    bool    m_is_reloading       = false;
    int32_t m_reload_total       = 0;
    int8_t  m_reload_wait_frames = 0;
};

} // namespace Ignis
