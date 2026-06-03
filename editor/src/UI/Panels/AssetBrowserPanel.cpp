#include "edpch.h"
#include "AssetBrowserPanel.h"
#include "Project/ProjectManager.h"
#include "Asset/AssetBrowserCommands.h"
#include <Ignis/Core/Shell.h>
#include <Ignis/Core/KeyCodes.h>
#include <Ignis/Input/InputSystem.h>

#ifdef ENGINE_IMGUI
#include <imgui.h>
#include "UI/ImExt/ImExt.h"
#endif

namespace Ignis
{

namespace
{

constexpr PanelId k_id             = 5;
constexpr float   k_thumbnail_size = 80.0f;

static constexpr InputMapping k_nav_mappings[] = {
    {AssetBrowserActions::k_nav_left, {Key::Left}},
    {AssetBrowserActions::k_nav_right, {Key::Right}},
    {AssetBrowserActions::k_nav_up, {Key::Up}},
    {AssetBrowserActions::k_nav_down, {Key::Down}},
    {AssetBrowserActions::k_activate, {Key::Enter}},
    {AssetBrowserActions::k_clipboard_copy, {Key::C, Modifier::Super}},
    {AssetBrowserActions::k_clipboard_cut, {Key::X, Modifier::Super}},
    {AssetBrowserActions::k_clipboard_paste, {Key::V, Modifier::Super}},
    {AssetBrowserActions::k_delete, {Key::Delete}},
};

ImGuiShortcutString get_shortcut(ActionID id)
{
    return {InputSystem::get_action_label(id)};
}

Vector<Path> get_asset_dependencies(const Path&)
{
    return {};
}
Vector<Path> get_asset_references(const Path&)
{
    return {};
}

String derive_type_label_for(const Path& path)
{
    String ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp")
    {
        return "TEX";
    }
    if (ext == ".hlsl" || ext == ".glsl" || ext == ".metal")
    {
        return "SHD";
    }
    if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb")
    {
        return "MSH";
    }
    if (ext == ".mat" || ext == ".igmat")
    {
        return "MAT";
    }
    if (ext == ".igscene")
    {
        return "SCN";
    }
    return "...";
}

} // namespace

AssetBrowserPanel::AssetBrowserPanel()
    : m_input_ctx("AssetBrowser", k_nav_mappings, std::size(k_nav_mappings), 50)
{
    InputSystem::register_context(&m_input_ctx);
}

AssetBrowserPanel::~AssetBrowserPanel()
{
    InputSystem::unregister_context(&m_input_ctx);
    m_model.shutdown();
}

PanelId AssetBrowserPanel::get_id() const
{
    return k_id;
}

void AssetBrowserPanel::update(float ts, IWorkspaceData* ctx)
{
    if (ctx)
    {
        m_dispatcher = static_cast<SceneEditorData*>(ctx)->dispatcher;
    }

    auto& pm = ProjectManager::get();
    if (pm.is_open())
    {
        if (m_is_reloading)
        {
            size_t active = EditorAssetManager::get().get_active_count();
            if (m_reload_total == 0 && active > 0)
            {
                m_reload_total = static_cast<int32_t>(active);
            }
            else if (active == 0 && (m_reload_total > 0 || ++m_reload_wait_frames >= 3))
            {
                m_is_reloading       = false;
                m_reload_total       = 0;
                m_reload_wait_frames = 0;
                m_model.refresh();
            }
        }

        auto& desc     = pm.descriptor();
        Path  new_root = desc.root / desc.asset_source_dir;
        if (new_root != m_model.assets_root())
        {
            m_model.shutdown();
            m_model.init(new_root);
            m_selected_paths.clear();
            m_last_clicked_idx = -1;
        }
        else
        {
            m_model.tick(ts);
            sync_selection();
        }

        EditorResourceCache::get().tick_thumbnails();
    }
    else if (!m_model.assets_root().empty())
    {
        m_model.shutdown();
        m_clipboard.clear();
        m_selected_paths.clear();
        m_renaming_path.clear();
        m_last_clicked_idx = -1;
    }
}

void AssetBrowserPanel::sync_selection()
{
    if (m_selected_paths.empty())
    {
        return;
    }

    const auto& entries = m_model.entries();
    m_selected_paths.erase(std::remove_if(m_selected_paths.begin(), m_selected_paths.end(),
                                          [&entries](const Path& sel)
                                          {
                                              for (const auto& e : entries)
                                              {
                                                  if (e.path == sel)
                                                  {
                                                      return false;
                                                  }
                                              }
                                              return true;
                                          }),
                           m_selected_paths.end());

    if (m_selected_paths.empty())
    {
        m_last_clicked_idx = -1;
    }
}

bool AssetBrowserPanel::is_selected(const Path& p) const
{
    for (const auto& s : m_selected_paths)
    {
        if (s == p)
        {
            return true;
        }
    }
    return false;
}

void AssetBrowserPanel::select_single(int32_t fi, const Vector<int32_t>& filtered)
{
    m_selected_paths.clear();
    m_selected_paths.push_back(m_model.entries()[filtered[fi]].path);
    m_last_clicked_idx = fi;
}

void AssetBrowserPanel::select_toggle(const Path& p)
{
    for (auto it = m_selected_paths.begin(); it != m_selected_paths.end(); ++it)
    {
        if (*it == p)
        {
            m_selected_paths.erase(it);
            return;
        }
    }
    m_selected_paths.push_back(p);
}

void AssetBrowserPanel::select_range(int32_t from_fi, int32_t to_fi, const Vector<int32_t>& filtered)
{
    int32_t lo = from_fi < to_fi ? from_fi : to_fi;
    int32_t hi = from_fi < to_fi ? to_fi : from_fi;
    m_selected_paths.clear();
    for (int32_t i = lo; i <= hi && i < (int32_t)filtered.size(); i++)
    {
        m_selected_paths.push_back(m_model.entries()[filtered[i]].path);
    }
}

void AssetBrowserPanel::push_recent(const Path& p)
{
    auto it = std::find(m_recent_files.begin(), m_recent_files.end(), p);
    if (it != m_recent_files.end())
    {
        m_recent_files.erase(it);
    }
    m_recent_files.push_front(p);
    if (m_recent_files.size() > 10)
    {
        m_recent_files.pop_back();
    }
}

void AssetBrowserPanel::draw(IWorkspaceData*)
{
#ifdef ENGINE_IMGUI
    if (m_model.current_dir().empty())
    {
        ImGui::TextDisabled("No project open.");
        return;
    }

    draw_breadcrumb();
    draw_favorites_bar();
    draw_toolbar();

    m_clipboard.draw_confirm_modal(m_dispatcher, &m_model);

    if (m_open_delete_confirm)
    {
        ImGui::OpenPopup("##delete_confirm");
        m_open_delete_confirm = false;
    }
    draw_delete_confirm_modal();

    const auto&     entries = m_model.entries();
    Vector<int32_t> filtered;
    filtered.reserve(entries.size());
    for (int32_t i = 0; i < (int32_t)entries.size(); i++)
    {
        filtered.push_back(i);
    }

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
    {
        ImGuiIO& io        = ImGui::GetIO();
        bool     mod       = io.KeySuper || io.KeyCtrl;
        bool     do_copy   = InputSystem::was_action_started(AssetBrowserActions::k_clipboard_copy) ||
                             (mod && ImGui::IsKeyPressed(ImGuiKey_C, false) && !io.WantTextInput);
        bool     do_cut    = InputSystem::was_action_started(AssetBrowserActions::k_clipboard_cut) ||
                             (mod && ImGui::IsKeyPressed(ImGuiKey_X, false) && !io.WantTextInput);
        bool     do_paste  = InputSystem::was_action_started(AssetBrowserActions::k_clipboard_paste) ||
                             (mod && ImGui::IsKeyPressed(ImGuiKey_V, false) && !io.WantTextInput);
        bool     do_delete = InputSystem::was_action_started(AssetBrowserActions::k_delete) ||
                             (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && !io.WantTextInput);

        if (do_copy)
        {
            m_clipboard.copy(m_selected_paths);
        }
        else if (do_cut)
        {
            m_clipboard.cut(m_selected_paths);
        }
        else if (do_paste)
        {
            m_clipboard.paste_into(m_model.current_dir(), m_dispatcher, &m_model);
        }
        else if (do_delete && !m_selected_paths.empty())
        {
            m_open_delete_confirm = true;
        }

        handle_keyboard_nav(filtered);
    }

    ImGui::Separator();

    constexpr float k_strip_h = 20.0f;
    float           content_h = ImGui::GetContentRegionAvail().y;
    float           table_h   = content_h - (m_is_reloading ? k_strip_h : 0.0f);

    constexpr ImGuiTableFlags k_split_flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit;
    if (ImGui::BeginTable("##browser_split_layout", 2, k_split_flags, {0.0f, table_h}))
    {
        ImGui::TableSetupColumn("##tree", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableSetupColumn("##content", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::BeginChild("##tree_scroll", {0.0f, content_h}, false, ImGuiWindowFlags_HorizontalScrollbar);
        draw_directory_tree();
        ImGui::EndChild();

        ImGui::TableSetColumnIndex(1);
        ImGui::BeginChild("##content_scroll", {0.0f, content_h}, ImGuiChildFlags_None);
        if (m_is_grid_view)
        {
            draw_grid_view(filtered);
        }
        else
        {
            draw_list_view(filtered);
        }
        draw_background_context_menu();
        draw_background_drop_target();
        ImGui::EndChild();

        ImGui::EndTable();
    }

    if (m_is_reloading)
    {
        int32_t active   = static_cast<int32_t>(EditorAssetManager::get().get_active_count());
        float   progress = m_reload_total > 0 ? float(m_reload_total - active) / float(m_reload_total) : 0.0f;
        ImGui::Separator();
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 6.0f));
    }

    if (m_open_add_tag)
    {
        ImGui::OpenPopup("##add_tag");
        m_open_add_tag = false;
    }
    draw_add_tag_modal();

    if (m_open_dep_popup)
    {
        ImGui::OpenPopup("##dep_view");
        m_open_dep_popup = false;
    }
    draw_dependency_popup();
#endif
}

void AssetBrowserPanel::draw_delete_confirm_modal()
{
#ifdef ENGINE_IMGUI
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, {0.5f, 0.5f});

    if (ImGui::BeginPopupModal("##delete_confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Delete %d item(s)?", (int32_t)m_selected_paths.size());
        ImGui::TextDisabled("Files move to .ignis_trash/ — restore via undo.");
        ImGui::Separator();

        if (ImGui::Button("Delete", {120.0f, 0.0f}))
        {
            Path trash = m_model.assets_root().parent_path() / ".ignis_trash";
            if (m_dispatcher)
            {
                m_dispatcher->commit(create_unique<DeleteAssetsCmd>(&m_model, m_selected_paths, trash));
            }
            else
            {
                std::error_code ec;
                Filesystem::create_directories(trash, ec);
                for (const auto& p : m_selected_paths)
                {
                    Filesystem::remove_all(p, ec);
                }
                m_model.refresh();
            }
            m_selected_paths.clear();
            m_last_clicked_idx = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", {120.0f, 0.0f}))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
#endif
}

void AssetBrowserPanel::draw_breadcrumb()
{
#ifdef ENGINE_IMGUI
    bool back_disabled = !m_model.can_go_back();
    if (back_disabled)
    {
        ImGui::BeginDisabled();
    }
    if (ImGui::SmallButton("<"))
    {
        m_model.navigate_back();
    }
    if (back_disabled)
    {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    bool fwd_disabled = !m_model.can_go_forward();
    if (fwd_disabled)
    {
        ImGui::BeginDisabled();
    }
    if (ImGui::SmallButton(">"))
    {
        m_model.navigate_forward();
    }
    if (fwd_disabled)
    {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    Path rel;
    {
        std::error_code ec;
        rel = Filesystem::relative(m_model.current_dir(), m_model.assets_root(), ec);
        if (ec)
        {
            rel = m_model.current_dir().filename();
        }
    }

    Path accumulated = m_model.assets_root();
    if (ImGui::SmallButton("Assets"))
    {
        m_model.navigate_to(m_model.assets_root());
    }

    for (const auto& seg : rel)
    {
        String seg_str = seg.string();
        if (seg_str == ".")
        {
            continue;
        }
        accumulated /= seg;
        ImGui::SameLine(0, 2);
        ImGui::TextDisabled("/");
        ImGui::SameLine(0, 2);
        Path nav_target = accumulated;
        if (ImGui::SmallButton(seg_str.c_str()))
        {
            m_model.navigate_to(nav_target);
        }
    }
#endif
}

void AssetBrowserPanel::draw_favorites_bar()
{
#ifdef ENGINE_IMGUI
    if (m_favorites.empty() && m_recent_files.empty())
    {
        return;
    }

    if (!m_favorites.empty())
    {
        ImGui::TextDisabled("Favs:");
        for (int32_t i = 0; i < (int32_t)m_favorites.size(); i++)
        {
            ImGui::SameLine();
            ImGui::PushID(i);
            String name = m_favorites[i].filename().string();
            if (ImGui::SmallButton(name.c_str()))
            {
                m_model.navigate_to(m_favorites[i]);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", m_favorites[i].string().c_str());
            }
            ImGui::PopID();
        }
        ImGui::NewLine();
    }

    if (!m_recent_files.empty())
    {
        ImGui::TextDisabled("Recents:");
        for (int32_t i = 0; i < (int32_t)m_recent_files.size(); i++)
        {
            ImGui::SameLine();
            ImGui::PushID(1000 + i);
            String name = m_recent_files[i].filename().string();
            if (ImGui::SmallButton(name.c_str()))
            {
                Path parent = m_recent_files[i].parent_path();
                if (parent != m_model.current_dir())
                {
                    m_model.navigate_to(parent);
                }
                for (const auto& entry : m_model.entries())
                {
                    if (entry.path == m_recent_files[i])
                    {
                        m_selected_paths.clear();
                        m_selected_paths.push_back(entry.path);
                        break;
                    }
                }
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", m_recent_files[i].string().c_str());
            }
            ImGui::PopID();
        }
        ImGui::NewLine();
    }
#endif
}

void AssetBrowserPanel::draw_toolbar()
{
#ifdef ENGINE_IMGUI
    float avail    = ImGui::GetContentRegionAvail().x;
    float toggle_w = 60.0f;
    float reload_w = 60.0f;
    float search_w = avail - toggle_w - reload_w - 24.0f;

    ImGui::SetNextItemWidth(search_w > 80.0f ? search_w : 80.0f);
    if (ImGui::InputText("##search", m_model.search_buf_mutable(), 256, ImGuiInputTextFlags_AutoSelectAll))
    {
        m_model.set_search(m_model.search_buf_mutable());
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(m_is_reloading);
    if (ImGui::SmallButton("Reload"))
    {
        InputSystem::trigger_action(EditorActions::k_reload_assets);
        m_is_reloading       = true;
        m_reload_total       = 0;
        m_reload_wait_frames = 0;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::SmallButton(m_is_grid_view ? "List" : "Grid"))
    {
        m_is_grid_view = !m_is_grid_view;
    }
#endif
}

void AssetBrowserPanel::draw_grid_view(const Vector<int32_t>& filtered)
{
#ifdef ENGINE_IMGUI
    const float pad     = 8.0f;
    const float cell_w  = k_thumbnail_size + pad;
    const float cell_h  = k_thumbnail_size + ImGui::GetTextLineHeightWithSpacing() + pad;
    const float avail_w = ImGui::GetContentRegionAvail().x;

    int32_t cols = (int32_t)(avail_w / cell_w);
    if (cols < 1)
    {
        cols = 1;
    }

    int32_t n    = (int32_t)filtered.size();
    int32_t rows = (n + cols - 1) / cols;

    ImGuiListClipper clipper;
    clipper.Begin(rows, cell_h);
    while (clipper.Step())
    {
        for (int32_t row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
        {
            for (int32_t col = 0; col < cols; col++)
            {
                int32_t fi = row * cols + col;
                if (fi >= n)
                {
                    break;
                }
                if (col > 0)
                {
                    ImGui::SameLine();
                }
                int32_t stable = filtered[fi];
                ImGui::PushID(stable);
                draw_entry(fi, filtered, m_model.entries()[stable]);
                ImGui::PopID();
            }
        }
    }
    clipper.End();
#endif
}

void AssetBrowserPanel::draw_list_view(const Vector<int32_t>& filtered)
{
#ifdef ENGINE_IMGUI
    int32_t col_count = 1;
    if (m_col_show_type)
    {
        col_count++;
    }
    if (m_col_show_size)
    {
        col_count++;
    }
    if (m_col_show_dirty)
    {
        col_count++;
    }
    if (m_col_show_tags)
    {
        col_count++;
    }

    constexpr ImGuiTableFlags k_flags =
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg;

    if (!ImGui::BeginTable("##assets", col_count, k_flags))
    {
        return;
    }

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
    if (m_col_show_type)
    {
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 42.0f);
    }
    if (m_col_show_size)
    {
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 72.0f);
    }
    if (m_col_show_dirty)
    {
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 48.0f);
    }
    if (m_col_show_tags)
    {
        ImGui::TableSetupColumn("Tags", ImGuiTableColumnFlags_WidthStretch);
    }
    ImGui::TableHeadersRow();

    float header_y0 = ImGui::GetItemRectMin().y;
    float header_y1 = ImGui::GetItemRectMax().y;
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsWindowHovered())
    {
        float my = ImGui::GetMousePos().y;
        if (my >= header_y0 && my <= header_y1)
        {
            ImGui::OpenPopup("##col_cfg");
        }
    }
    if (ImGui::BeginPopup("##col_cfg"))
    {
        ImGui::MenuItem("Type", nullptr, &m_col_show_type);
        ImGui::MenuItem("Size", nullptr, &m_col_show_size);
        ImGui::MenuItem("Dirty State", nullptr, &m_col_show_dirty);
        ImGui::MenuItem("Tags", nullptr, &m_col_show_tags);
        ImGui::EndPopup();
    }

    const auto&      entries = m_model.entries();
    ImGuiListClipper clipper;
    clipper.Begin((int32_t)filtered.size());
    while (clipper.Step())
    {
        for (int32_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            int32_t     stable = filtered[i];
            const auto& de     = entries[stable];
            ImGui::PushID(stable);
            ImGui::TableNextRow();

            int32_t col = 0;
            ImGui::TableSetColumnIndex(col++);
            draw_entry(i, filtered, de);

            if (m_col_show_type)
            {
                ImGui::TableSetColumnIndex(col++);
                ImGui::TextUnformatted(de.type_label.c_str());
            }
            if (m_col_show_size)
            {
                ImGui::TableSetColumnIndex(col++);
                if (de.is_dir)
                {
                    ImGui::TextDisabled("--");
                }
                else
                {
                    ImGui::Text("%.1f KB", (double)de.file_size / 1024.0);
                }
            }
            if (m_col_show_dirty)
            {
                ImGui::TableSetColumnIndex(col++);
                if (de.status_missing)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 70, 70, 255));
                    ImGui::TextUnformatted("!");
                    ImGui::PopStyleColor();
                }
            }
            if (m_col_show_tags)
            {
                ImGui::TableSetColumnIndex(col++);
                auto tag_it = m_asset_tags.find(de.path.string());
                if (tag_it != m_asset_tags.end() && !tag_it->second.empty())
                {
                    String tags_line;
                    for (const auto& t : tag_it->second)
                    {
                        tags_line += (tags_line.empty() ? "#" : " #") + t;
                    }
                    ImGui::TextDisabled("%s", tags_line.c_str());
                }
            }

            ImGui::PopID();
        }
    }
    clipper.End();
    ImGui::EndTable();
#endif
}

void AssetBrowserPanel::draw_entry(int32_t fi, const Vector<int32_t>& filtered, const AssetDirectoryModel::Entry& e)
{
#ifdef ENGINE_IMGUI
    bool selected = is_selected(e.path);

    if (m_is_grid_view)
    {
        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        bool renaming = (e.path == m_renaming_path);

        if (renaming)
        {
            ImGui::SetNextItemWidth(k_thumbnail_size);
            if (ImGui::InputText("##rename", m_rename_buf, sizeof(m_rename_buf),
                                 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                commit_rename();
            }
            if (ImGui::IsItemDeactivated() && !ImGui::IsItemActive())
            {
                commit_rename();
            }
        }
        else
        {
            ImGui::PushID(e.path.string().c_str());
            ImGui::BeginGroup();

            bool clicked = false;
            if (e.is_dir)
            {
                GRITexture2D* icon   = EditorResourceCache::get().get_folder_icon();
                ImTextureID   tex_id = icon ? reinterpret_cast<ImTextureID>(icon->get_native_handle()) : ImTextureID{};
                ImVec2        btn_sz = {k_thumbnail_size, k_thumbnail_size};
                clicked = tex_id ? ImGui::ImageButton("#i", tex_id, btn_sz) : ImGui::Button("[DIR]", btn_sz);
            }
            else
            {
                const ImVec2 btn_sz{k_thumbnail_size, k_thumbnail_size};
                bool         is_thumbable =
                    (e.type_label == "TEX" || e.type_label == "MSH" || e.type_label == "MAT") && !e.status_missing;
                if (is_thumbable)
                {
                    auto tr = EditorResourceCache::get().request_thumbnail(e.path, e.type_label);
                    if (tr.ready)
                    {
                        clicked = ImGui::ImageButton("#t", reinterpret_cast<ImTextureID>(tr.tex_id), btn_sz,
                                                     {tr.uv0[0], tr.uv0[1]}, {tr.uv1[0], tr.uv1[1]});
                    }
                    else
                    {
                        GRITexture2D* ph  = EditorResourceCache::get().get_type_icon(e.type_label);
                        ImTextureID ph_id = ph ? reinterpret_cast<ImTextureID>(ph->get_native_handle()) : ImTextureID{};
                        clicked           = ph_id ? ImGui::ImageButton("#i", ph_id, btn_sz)
                                                  : ImGui::Button(("[" + e.type_label + "]").c_str(), btn_sz);
                    }
                }
                else
                {
                    GRITexture2D* icon = EditorResourceCache::get().get_type_icon(e.type_label);
                    ImTextureID   icon_id =
                        icon ? reinterpret_cast<ImTextureID>(icon->get_native_handle()) : ImTextureID{};
                    clicked = icon_id ? ImGui::ImageButton("#i", icon_id, btn_sz)
                                      : ImGui::Button(("[" + e.type_label + "]").c_str(), btn_sz);
                }
            }

            ImVec2 btn_rmin      = ImGui::GetItemRectMin();
            ImVec2 btn_rmax      = ImGui::GetItemRectMax();
            bool   right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);

            if (clicked || right_clicked)
            {
                ImGuiIO& io = ImGui::GetIO();
                if (io.KeySuper || io.KeyCtrl)
                {
                    select_toggle(e.path);
                }
                else if (io.KeyShift && m_last_clicked_idx >= 0)
                {
                    select_range(m_last_clicked_idx, fi, filtered);
                }
                else if (!right_clicked || !is_selected(e.path))
                {
                    select_single(fi, filtered);
                }
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                if (e.is_dir)
                {
                    m_model.navigate_to(e.path);
                }
                else
                {
                    push_recent(e.path);
                }
            }

            if (ImExt::DragDrop::begin_source())
            {
                AssetDragPayload    payload{};
                std::error_code     ec;
                const Vector<Path>& drag_set = is_selected(e.path) ? m_selected_paths : Vector<Path>{e.path};

                for (const auto& src : drag_set)
                {
                    if (payload.count >= AssetDragPayload::k_max_items)
                    {
                        break;
                    }
                    Path rel = Filesystem::relative(src, m_model.assets_root(), ec);
                    if (ec)
                    {
                        rel = src;
                    }
                    String rel_str = rel.string();
                    String ext     = src.extension().string();
                    String tlabel  = derive_type_label_for(src);

                    auto& item = payload.items[payload.count++];
                    IG_ASSERT(rel_str.size() < sizeof(item.rel_path), "Asset path exceeds payload buffer");
                    std::strncpy(item.rel_path, rel_str.c_str(), sizeof(item.rel_path) - 1);
                    std::strncpy(item.extension, ext.c_str(), sizeof(item.extension) - 1);
                    std::strncpy(item.type_label, tlabel.c_str(), sizeof(item.type_label) - 1);
                }

                ImExt::DragDrop::set_payload(payload);
                if (payload.count > 1)
                {
                    int32_t mesh_c = 0, tex_c = 0, mat_c = 0, other_c = 0;
                    for (uint32_t i = 0; i < payload.count; ++i)
                    {
                        const char* tl = payload.items[i].type_label;
                        if (std::strcmp(tl, "MSH") == 0)
                        {
                            ++mesh_c;
                        }
                        else if (std::strcmp(tl, "TEX") == 0)
                        {
                            ++tex_c;
                        }
                        else if (std::strcmp(tl, "MAT") == 0)
                        {
                            ++mat_c;
                        }
                        else
                        {
                            ++other_c;
                        }
                    }
                    if (mesh_c)
                    {
                        ImGui::Text("%d Mesh", mesh_c);
                    }
                    if (tex_c)
                    {
                        ImGui::Text("%d Texture", tex_c);
                    }
                    if (mat_c)
                    {
                        ImGui::Text("%d Material", mat_c);
                    }
                    if (other_c)
                    {
                        ImGui::Text("%d Other", other_c);
                    }
                }
                else
                {
                    ImGui::Text("%s  [%s]", e.display_name.c_str(), e.type_label.c_str());
                }
                ImExt::DragDrop::end_source();
            }

            if (e.is_dir && ImExt::DragDrop::begin_target())
            {
                bool  is_copy = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
                ImU32 col     = is_copy ? IM_COL32(100, 200, 100, 200) : IM_COL32(100, 150, 255, 200);
                ImGui::GetWindowDrawList()->AddRect(btn_rmin, btn_rmax, col, 4.0f, 0, 2.0f);
                ImGui::SetTooltip("%s into \"%s\"", is_copy ? "Copy" : "Move", e.display_name.c_str());
                if (const auto* p = ImExt::DragDrop::accept<AssetDragPayload>())
                {
                    drop_into_dir(p, e.path, is_copy);
                }
                ImExt::DragDrop::end_target();
            }

            ImGui::OpenPopupOnItemClick("##entry_ctx", ImGuiPopupFlags_MouseButtonRight);

            String truncated = e.display_name;
            if (truncated.size() > 12)
            {
                truncated = truncated.substr(0, 11) + "~";
            }
            ImGui::TextUnformatted(truncated.c_str());

            ImGui::EndGroup();

            if (ImGui::BeginPopup("##entry_ctx"))
            {
                draw_entry_context_menu(e);
                ImGui::EndPopup();
            }

            ImGui::PopID();
            draw_asset_status_badge(e);
            if (ImGui::IsItemHovered())
            {
                draw_hover_tooltip(e);
            }
        }

        if (selected)
        {
            ImGui::PopStyleColor();
        }
    }
    else
    {
        bool renaming    = (e.path == m_renaming_path);
        bool clicked     = ImGui::Selectable(renaming ? "##sel" : e.display_name.c_str(), selected,
                                             ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
        bool sel_hovered = !renaming && ImGui::IsItemHovered();
        draw_asset_status_badge(e);

        if (renaming)
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText("##rename", m_rename_buf, sizeof(m_rename_buf),
                                 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                commit_rename();
            }
            if (ImGui::IsItemDeactivated() && !ImGui::IsItemActive())
            {
                commit_rename();
            }
        }

        bool right_clicked = !renaming && ImGui::IsItemClicked(ImGuiMouseButton_Right);

        if (clicked || right_clicked)
        {
            ImGuiIO& io = ImGui::GetIO();
            if (io.KeySuper || io.KeyCtrl)
            {
                select_toggle(e.path);
            }
            else if (io.KeyShift && m_last_clicked_idx >= 0)
            {
                select_range(m_last_clicked_idx, fi, filtered);
            }
            else if (!right_clicked || !is_selected(e.path))
            {
                select_single(fi, filtered);
            }
        }

        if (sel_hovered && ImGui::IsMouseDoubleClicked(0))
        {
            if (e.is_dir)
            {
                m_model.navigate_to(e.path);
            }
            else
            {
                push_recent(e.path);
            }
        }

        if (!renaming && ImExt::DragDrop::begin_source())
        {
            AssetDragPayload    payload{};
            std::error_code     ec;
            const Vector<Path>& drag_set = is_selected(e.path) ? m_selected_paths : Vector<Path>{e.path};

            for (const auto& src : drag_set)
            {
                if (payload.count >= AssetDragPayload::k_max_items)
                {
                    break;
                }
                Path rel = Filesystem::relative(src, m_model.assets_root(), ec);
                if (ec)
                {
                    rel = src;
                }
                String rel_str = rel.string();
                String ext     = src.extension().string();
                String tlabel  = derive_type_label_for(src);

                auto& item = payload.items[payload.count++];
                IG_ASSERT(rel_str.size() < sizeof(item.rel_path), "Asset path exceeds payload buffer");
                std::strncpy(item.rel_path, rel_str.c_str(), sizeof(item.rel_path) - 1);
                std::strncpy(item.extension, ext.c_str(), sizeof(item.extension) - 1);
                std::strncpy(item.type_label, tlabel.c_str(), sizeof(item.type_label) - 1);
            }

            ImExt::DragDrop::set_payload(payload);
            if (payload.count > 1)
            {
                int32_t mesh_c = 0, tex_c = 0, mat_c = 0, other_c = 0;
                for (uint32_t i = 0; i < payload.count; ++i)
                {
                    const char* tl = payload.items[i].type_label;
                    if (std::strcmp(tl, "MSH") == 0)
                    {
                        ++mesh_c;
                    }
                    else if (std::strcmp(tl, "TEX") == 0)
                    {
                        ++tex_c;
                    }
                    else if (std::strcmp(tl, "MAT") == 0)
                    {
                        ++mat_c;
                    }
                    else
                    {
                        ++other_c;
                    }
                }
                if (mesh_c)
                {
                    ImGui::Text("%d Mesh", mesh_c);
                }
                if (tex_c)
                {
                    ImGui::Text("%d Texture", tex_c);
                }
                if (mat_c)
                {
                    ImGui::Text("%d Material", mat_c);
                }
                if (other_c)
                {
                    ImGui::Text("%d Other", other_c);
                }
            }
            else
            {
                ImGui::Text("%s  [%s]", e.display_name.c_str(), e.type_label.c_str());
            }
            ImExt::DragDrop::end_source();
        }

        if (!renaming && e.is_dir && ImExt::DragDrop::begin_target())
        {
            bool   is_copy = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
            ImVec2 rmin    = ImGui::GetItemRectMin();
            ImVec2 rmax    = ImGui::GetItemRectMax();
            ImU32  col     = is_copy ? IM_COL32(100, 200, 100, 200) : IM_COL32(100, 150, 255, 200);
            ImGui::GetWindowDrawList()->AddRect(rmin, rmax, col, 2.0f, 0, 2.0f);
            ImGui::SetTooltip("%s into \"%s\"", is_copy ? "Copy" : "Move", e.display_name.c_str());
            if (const auto* p = ImExt::DragDrop::accept<AssetDragPayload>())
            {
                drop_into_dir(p, e.path, is_copy);
            }
            ImExt::DragDrop::end_target();
        }

        if (e.path != m_renaming_path)
        {
            ImGui::OpenPopupOnItemClick("##entry_ctx", ImGuiPopupFlags_MouseButtonRight);
        }

        if (sel_hovered)
        {
            draw_hover_tooltip(e);
        }
    }

    if (ImGui::BeginPopup("##entry_ctx"))
    {
        draw_entry_context_menu(e);
        ImGui::EndPopup();
    }
#endif
}

void AssetBrowserPanel::draw_entry_context_menu(const AssetDirectoryModel::Entry& e)
{
#ifdef ENGINE_IMGUI
    if (ImGui::MenuItem("Rename"))
    {
        m_renaming_path = e.path;
        std::strncpy(m_rename_buf, e.display_name.c_str(), sizeof(m_rename_buf) - 1);
        m_rename_buf[sizeof(m_rename_buf) - 1] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }
    if (ImGui::MenuItem("Delete", get_shortcut(AssetBrowserActions::k_delete)))
    {
        if (!m_selected_paths.empty())
        {
            m_open_delete_confirm = true;
        }
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Copy", get_shortcut(AssetBrowserActions::k_clipboard_copy)))
    {
        m_clipboard.copy(m_selected_paths);
    }
    if (ImGui::MenuItem("Cut", get_shortcut(AssetBrowserActions::k_clipboard_cut)))
    {
        m_clipboard.cut(m_selected_paths);
    }
    if (ImGui::MenuItem("Paste", get_shortcut(AssetBrowserActions::k_clipboard_paste), false,
                        e.is_dir && m_clipboard.has_content()))
    {
        m_clipboard.paste_into(e.path, m_dispatcher, &m_model);
    }

    ImGui::Separator();
    auto fav_it = std::find(m_favorites.begin(), m_favorites.end(), e.path);
    if (fav_it != m_favorites.end())
    {
        if (ImGui::MenuItem("Unpin from Favorites"))
        {
            if (m_dispatcher)
            {
                m_dispatcher->commit(create_unique<UnpinFavoriteCmd>(&m_favorites, e.path));
            }
            else
            {
                m_favorites.erase(fav_it);
            }
        }
    }
    else
    {
        if (ImGui::MenuItem("Pin to Favorites"))
        {
            if (m_dispatcher)
            {
                m_dispatcher->commit(create_unique<PinFavoriteCmd>(&m_favorites, e.path));
            }
            else
            {
                m_favorites.push_back(e.path);
            }
        }
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Add Tag..."))
    {
        m_tag_target   = e.path;
        m_tag_buf[0]   = '\0';
        m_open_add_tag = true;
    }

    auto tag_it = m_asset_tags.find(e.path.string());
    if (tag_it != m_asset_tags.end() && !tag_it->second.empty())
    {
        ImGui::TextDisabled("Tags:");
        for (int32_t i = 0; i < (int32_t)tag_it->second.size(); i++)
        {
            ImGui::PushID(i);
            String label = tag_it->second[i] + " [x]";
            if (ImGui::MenuItem(label.c_str()))
            {
                String removed_tag = tag_it->second[i];
                if (m_dispatcher)
                {
                    m_dispatcher->commit(create_unique<RemoveTagCmd>(&m_asset_tags, e.path.string(), removed_tag));
                }
                else
                {
                    tag_it->second.erase(tag_it->second.begin() + i);
                }
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Show Dependencies..."))
    {
        m_dep_popup_target = e.path;
        m_open_dep_popup   = true;
    }
    if (ImGui::MenuItem("Show in Finder"))
    {
        Shell::reveal_in_file_manager(e.path.parent_path());
    }
#endif
}

void AssetBrowserPanel::draw_background_context_menu()
{
#ifdef ENGINE_IMGUI
    static char s_new_folder_buf[256] = {};

    if (ImGui::BeginPopupContextWindow("##bg_ctx", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Folder"))
        {
            s_new_folder_buf[0] = '\0';
            ImGui::OpenPopup("##new_folder");
        }
        if (m_clipboard.has_content() && ImGui::MenuItem("Paste", get_shortcut(AssetBrowserActions::k_clipboard_paste)))
        {
            m_clipboard.paste_into(m_model.current_dir(), m_dispatcher, &m_model);
        }
        ImGui::EndPopup();
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, {0.5f, 0.5f});
    if (ImGui::BeginPopupModal("##new_folder", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
    {
        ImGui::TextUnformatted("Folder name:");
        ImGui::SetNextItemWidth(200.0f);
        bool confirm = ImGui::InputText("##foldername", s_new_folder_buf, sizeof(s_new_folder_buf),
                                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        ImGui::SameLine();
        confirm |= ImGui::Button("Create");
        if (confirm && s_new_folder_buf[0] != '\0')
        {
            Path new_dir = m_model.current_dir() / s_new_folder_buf;
            if (m_dispatcher)
            {
                m_dispatcher->commit(create_unique<CreateFolderCmd>(&m_model, new_dir));
            }
            else
            {
                std::error_code ec;
                Filesystem::create_directory(new_dir, ec);
                m_model.refresh();
            }
            s_new_folder_buf[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            s_new_folder_buf[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
#endif
}

void AssetBrowserPanel::draw_add_tag_modal()
{
#ifdef ENGINE_IMGUI
    if (!ImGui::BeginPopup("##add_tag"))
    {
        return;
    }

    ImGui::TextUnformatted("Tag name:");
    ImGui::SetNextItemWidth(150.0f);
    bool confirm = ImGui::InputText("##tagname", m_tag_buf, sizeof(m_tag_buf),
                                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
    ImGui::SameLine();
    confirm |= ImGui::Button("Add");
    if (confirm && m_tag_buf[0] != '\0')
    {
        if (m_dispatcher)
        {
            m_dispatcher->commit(create_unique<AddTagCmd>(&m_asset_tags, m_tag_target.string(), String(m_tag_buf)));
        }
        else
        {
            m_asset_tags[m_tag_target.string()].push_back(m_tag_buf);
        }
        m_tag_buf[0] = '\0';
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
#endif
}

void AssetBrowserPanel::draw_dependency_popup()
{
#ifdef ENGINE_IMGUI
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({520.0f, 360.0f}, ImGuiCond_Appearing);
    if (!ImGui::BeginPopupModal("##dep_view", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
    {
        return;
    }

    ImGui::TextUnformatted(m_dep_popup_target.filename().string().c_str());
    ImGui::Separator();

    Vector<Path> refs = get_asset_references(m_dep_popup_target);
    Vector<Path> deps = get_asset_dependencies(m_dep_popup_target);

    float close_h = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
    float list_h  = ImGui::GetContentRegionAvail().y - close_h;
    float half_w  = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

    auto draw_path_list = [&](const Vector<Path>& paths, const char* child_id)
    {
        ImGui::BeginChild(child_id, {half_w, list_h}, true);
        if (paths.empty())
        {
            ImGui::TextDisabled("None");
        }
        else
        {
            for (int32_t i = 0; i < (int32_t)paths.size(); i++)
            {
                ImGui::PushID(i);
                String name = paths[i].filename().string();
                if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick) &&
                    ImGui::IsMouseDoubleClicked(0))
                {
                    ImGui::CloseCurrentPopup();
                    m_model.navigate_to(paths[i].parent_path());
                    m_selected_paths.clear();
                    m_selected_paths.push_back(paths[i]);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", paths[i].string().c_str());
                }
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
    };

    ImGui::BeginGroup();
    ImGui::TextDisabled("References  (%d)", (int32_t)refs.size());
    draw_path_list(refs, "##dep_refs");
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::TextDisabled("Dependencies  (%d)", (int32_t)deps.size());
    draw_path_list(deps, "##dep_deps");
    ImGui::EndGroup();

    if (ImGui::Button("Close"))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
#endif
}

void AssetBrowserPanel::draw_asset_status_badge(const AssetDirectoryModel::Entry& e) const
{
#ifdef ENGINE_IMGUI
    if (!e.status_missing)
    {
        return;
    }

    ImVec2 rmin = ImGui::GetItemRectMin();
    ImVec2 rmax = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddCircleFilled({rmax.x - 6.0f, rmin.y + 6.0f}, 4.0f, IM_COL32(220, 70, 70, 230));
#endif
}

void AssetBrowserPanel::draw_hover_tooltip(const AssetDirectoryModel::Entry& e) const
{
#ifdef ENGINE_IMGUI
    ImGui::BeginTooltip();

    auto draw_preview = [&](const ImVec2& sz)
    {
        auto tr = EditorResourceCache::get().request_thumbnail(e.path, e.type_label);
        if (tr.ready)
        {
            ImGui::Image(reinterpret_cast<ImTextureID>(tr.tex_id), sz, {tr.uv0[0], tr.uv0[1]}, {tr.uv1[0], tr.uv1[1]});
        }
        else
        {
            GRITexture2D* ph = EditorResourceCache::get().get_type_icon(e.type_label);
            if (ph)
            {
                ImGui::Image(reinterpret_cast<ImTextureID>(ph->get_native_handle()), sz);
            }
        }
    };

    constexpr ImVec2 k_prev_sz = {80.0f, 80.0f};

    if (e.type_label == "TEX" || e.type_label == "MSH" || e.type_label == "MAT")
    {
        draw_preview(k_prev_sz);
        ImGui::Separator();
    }

    if (e.type_label == "SCN")
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 180, 255, 255));
        ImGui::TextUnformatted("Scene");
        ImGui::PopStyleColor();
    }
    else if (e.type_label == "SHD")
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 130, 255, 255));
        ImGui::TextUnformatted("Shader");
        ImGui::PopStyleColor();
    }
    else if (e.type_label == "MSH")
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 220, 130, 255));
        ImGui::TextUnformatted("Mesh");
        ImGui::PopStyleColor();
    }
    else if (e.type_label == "MAT")
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 180, 80, 255));
        ImGui::TextUnformatted("Material");
        ImGui::PopStyleColor();
    }
    else if (e.is_dir)
    {
        ImGui::TextUnformatted(e.display_name.c_str());
        ImGui::Separator();
        ImGui::TextDisabled("Directory");
        ImGui::EndTooltip();
        return;
    }

    ImGui::TextUnformatted(e.display_name.c_str());
    if (e.type_label == "TEX")
    {
        ImGui::TextDisabled("Dimensions:  ? x ?");
    }
    ImGui::TextDisabled("Size:  %.1f KB", (double)e.file_size / 1024.0);

    auto tag_it = m_asset_tags.find(e.path.string());
    if (tag_it != m_asset_tags.end() && !tag_it->second.empty())
    {
        ImGui::Separator();
        String tags_line;
        for (const auto& t : tag_it->second)
        {
            tags_line += (tags_line.empty() ? "#" : "  #") + t;
        }
        ImGui::TextDisabled("%s", tags_line.c_str());
    }

    if (e.status_missing)
    {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 70, 70, 255));
        ImGui::TextUnformatted("! Missing from disk");
        ImGui::PopStyleColor();
    }

    ImGui::EndTooltip();
#endif
}

void AssetBrowserPanel::handle_keyboard_nav(const Vector<int32_t>& filtered)
{
    if (filtered.empty())
    {
        return;
    }

    int32_t n     = (int32_t)filtered.size();
    int32_t prev  = (m_last_clicked_idx >= 0 && m_last_clicked_idx < n) ? m_last_clicked_idx : -1;
    int32_t next  = prev;
    bool    moved = false;

    if (m_is_grid_view)
    {
        int32_t cols = (int32_t)(ImGui::GetContentRegionAvail().x / (k_thumbnail_size + 8.0f));
        if (cols < 1)
        {
            cols = 1;
        }
        if (InputSystem::was_action_started(AssetBrowserActions::k_nav_right))
        {
            next  = (prev < 0) ? 0 : std::min(prev + 1, n - 1);
            moved = true;
        }
        if (InputSystem::was_action_started(AssetBrowserActions::k_nav_left))
        {
            next  = (prev < 0) ? 0 : std::max(prev - 1, 0);
            moved = true;
        }
        if (InputSystem::was_action_started(AssetBrowserActions::k_nav_down))
        {
            next  = (prev < 0) ? 0 : std::min(prev + cols, n - 1);
            moved = true;
        }
        if (InputSystem::was_action_started(AssetBrowserActions::k_nav_up))
        {
            next  = (prev < 0) ? 0 : std::max(prev - cols, 0);
            moved = true;
        }
    }
    else
    {
        if (InputSystem::was_action_started(AssetBrowserActions::k_nav_down))
        {
            next  = (prev < 0) ? 0 : std::min(prev + 1, n - 1);
            moved = true;
        }
        if (InputSystem::was_action_started(AssetBrowserActions::k_nav_up))
        {
            next  = (prev < 0) ? 0 : std::max(prev - 1, 0);
            moved = true;
        }
    }

    if (moved)
    {
        select_single(next, filtered);
    }

    if (InputSystem::was_action_started(AssetBrowserActions::k_activate) && m_selected_paths.size() == 1)
    {
        for (const auto& e : m_model.entries())
        {
            if (e.path == m_selected_paths[0])
            {
                if (e.is_dir)
                {
                    m_model.navigate_to(e.path);
                }
                else
                {
                    push_recent(e.path);
                }
                break;
            }
        }
    }
}

void AssetBrowserPanel::draw_directory_tree()
{
#ifdef ENGINE_IMGUI
    if (m_model.assets_root().empty())
    {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.0f);

    String project_name  = ProjectManager::get().descriptor().name;
    bool   root_selected = (m_model.current_dir() == m_model.assets_root());

    ImGuiTreeNodeFlags root_flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
    if (root_selected)
    {
        root_flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool root_open = ImGui::TreeNodeEx(project_name.c_str(), root_flags);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
    {
        m_model.navigate_to(m_model.assets_root());
    }

    if (root_open)
    {
        draw_directory_tree_node(m_model.assets_root());
        ImGui::TreePop();
    }

    ImGui::PopStyleVar();
#endif
}

void AssetBrowserPanel::draw_directory_tree_node(const Path& dir)
{
#ifdef ENGINE_IMGUI
    std::error_code ec;
    struct TreeItem
    {
        Path path;
        bool is_dir;
    };
    Vector<TreeItem> items;
    for (const auto& entry : Filesystem::directory_iterator(dir, ec))
    {
        if (!ec)
        {
            items.push_back({entry.path(), entry.is_directory()});
        }
    }
    std::sort(items.begin(), items.end(),
              [](const TreeItem& a, const TreeItem& b)
              {
                  if (a.is_dir != b.is_dir)
                  {
                      return a.is_dir > b.is_dir;
                  }
                  return a.path.filename() < b.path.filename();
              });

    for (const auto& item : items)
    {
        String name = item.path.filename().string();

        if (item.is_dir)
        {
            bool is_current      = (item.path == m_model.current_dir());
            bool on_current_path = false;
            {
                Path tmp = m_model.current_dir();
                while (tmp != m_model.assets_root() && tmp != tmp.parent_path())
                {
                    if (tmp == item.path)
                    {
                        on_current_path = true;
                        break;
                    }
                    tmp = tmp.parent_path();
                }
            }

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (is_current)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            if (on_current_path || is_current)
            {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
            }

            bool open = ImGui::TreeNodeEx(name.c_str(), flags);
            {
                ImVec2 rmin = ImGui::GetItemRectMin();
                ImVec2 rmax = ImGui::GetItemRectMax();
                if (ImExt::DragDrop::begin_target())
                {
                    bool  is_copy = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
                    ImU32 col     = is_copy ? IM_COL32(100, 200, 100, 200) : IM_COL32(100, 150, 255, 200);
                    ImGui::GetWindowDrawList()->AddRect(rmin, rmax, col, 2.0f, 0, 2.0f);
                    ImGui::SetTooltip("%s into \"%s\"", is_copy ? "Copy" : "Move", name.c_str());
                    if (const auto* p = ImExt::DragDrop::accept<AssetDragPayload>())
                    {
                        drop_into_dir(p, item.path, is_copy);
                    }
                    ImExt::DragDrop::end_target();
                }
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
            {
                m_model.navigate_to(item.path);
            }

            if (open)
            {
                draw_directory_tree_node(item.path);
                ImGui::TreePop();
            }
        }
        else
        {
            bool               is_sel = is_selected(item.path);
            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (is_sel)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            ImGui::TreeNodeEx(name.c_str(), flags);

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                m_selected_paths.clear();
                m_selected_paths.push_back(item.path);
                m_last_clicked_idx = -1;
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                push_recent(item.path);
            }
        }
    }
#endif
}

void AssetBrowserPanel::drop_into_dir(const AssetDragPayload* p, const Path& dest, bool is_copy)
{
    auto is_cycle = [](const Path& src, const Path& dst) -> bool
    {
        if (src == dst)
        {
            return true;
        }
        std::error_code ec;
        Path            rel = Filesystem::relative(dst, src, ec);
        if (ec)
        {
            return true;
        }
        auto it = rel.begin();
        return it != rel.end() && it->string() != "..";
    };

    Vector<Path> srcs;
    srcs.reserve(p->count);
    for (uint32_t i = 0; i < p->count; i++)
    {
        Path abs_src = m_model.assets_root() / p->items[i].rel_path;
        if (abs_src.parent_path() == dest || is_cycle(abs_src, dest))
        {
            continue;
        }
        srcs.push_back(abs_src);
    }

    if (srcs.empty())
    {
        return;
    }

    if (m_dispatcher)
    {
        m_dispatcher->commit(create_unique<PasteAssetsCmd>(&m_model, srcs, dest, !is_copy, false));
    }
    else
    {
        std::error_code ec;
        for (const auto& src : srcs)
        {
            Path dst = dest / src.filename();
            if (is_copy)
            {
                Filesystem::copy(
                    src, dst, Filesystem::copy_options::recursive | Filesystem::copy_options::overwrite_existing, ec);
            }
            else
            {
                Filesystem::rename(src, dst, ec);
            }
        }
        m_model.refresh();
    }
}

void AssetBrowserPanel::draw_background_drop_target()
{
#ifdef ENGINE_IMGUI
    if (!ImGui::GetDragDropPayload() || ImGui::IsAnyItemHovered())
    {
        return;
    }

    if (ImExt::DragDrop::begin_window_target())
    {
        bool         is_copy = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
        ImU32        tint    = is_copy ? IM_COL32(100, 200, 100, 30) : IM_COL32(100, 150, 255, 30);
        ImGuiWindow* w       = ImGui::GetCurrentWindow();
        w->DrawList->AddRectFilled(w->Rect().Min, w->Rect().Max, tint);
        ImGui::SetTooltip("%s into current folder", is_copy ? "Copy" : "Move");
        if (const auto* p = ImExt::DragDrop::accept<AssetDragPayload>())
        {
            drop_into_dir(p, m_model.current_dir(), is_copy);
        }
        ImGui::EndDragDropTarget();
    }
#endif
}

void AssetBrowserPanel::commit_rename()
{
    if (m_renaming_path.empty() || m_rename_buf[0] == '\0')
    {
        m_renaming_path.clear();
        return;
    }

    Path new_path = m_renaming_path.parent_path() / m_rename_buf;
    if (new_path != m_renaming_path)
    {
        if (m_dispatcher)
        {
            m_dispatcher->commit(create_unique<RenameAssetCmd>(&m_model, m_renaming_path, new_path));
        }
        else
        {
            std::error_code ec;
            Filesystem::rename(m_renaming_path, new_path, ec);
            m_model.refresh();
        }
    }
    m_renaming_path.clear();
}

} // namespace Ignis
