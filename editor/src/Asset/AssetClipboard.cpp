#include "edpch.h"
#include "AssetClipboard.h"
#include "AssetDirectoryModel.h"
#include "AssetBrowserCommands.h"

#ifdef ENGINE_IMGUI
#endif

namespace Ignis
{

void AssetClipboard::copy(const Vector<Path>& paths)
{
    m_clipboard = paths;
    m_is_cut    = false;
}

void AssetClipboard::cut(const Vector<Path>& paths)
{
    m_clipboard = paths;
    m_is_cut    = true;
    // No filesystem ops — the move happens at paste time so undo can restore to origin.
}

void AssetClipboard::paste_into(const Path& dest, CommandDispatcher* dispatcher, AssetDirectoryModel* model)
{
    if (m_clipboard.empty())
    {
        return;
    }

    bool conflict = false;
    for (const auto& src : m_clipboard)
    {
        if (Filesystem::exists(dest / src.filename()))
        {
            conflict = true;
            break;
        }
    }

    if (conflict)
    {
        m_pending_srcs   = m_clipboard;
        m_pending_dest   = dest;
        m_pending_is_cut = m_is_cut;
        m_open_confirm   = true;
    }
    else
    {
        execute_paste(m_clipboard, dest, m_is_cut, false, dispatcher, model);
    }
}

void AssetClipboard::draw_confirm_modal(CommandDispatcher* dispatcher, AssetDirectoryModel* model)
{
#ifdef ENGINE_IMGUI
    if (m_open_confirm)
    {
        ImGui::OpenPopup("##paste_confirm");
        m_open_confirm = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, {0.5f, 0.5f});

    if (ImGui::BeginPopupModal("##paste_confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("One or more files already exist in the target folder.");
        ImGui::TextUnformatted("Do you want to completely overwrite the existing items?");
        ImGui::Separator();

        if (ImGui::Button("Overwrite", {120.0f, 0.0f}))
        {
            execute_paste(m_pending_srcs, m_pending_dest, m_pending_is_cut, true, dispatcher, model);
            m_pending_srcs.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", {120.0f, 0.0f}))
        {
            m_pending_srcs.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
#endif
}

void AssetClipboard::execute_paste(const Vector<Path>& srcs, const Path& dest, bool is_cut, bool overwrite,
                                   CommandDispatcher* dispatcher, AssetDirectoryModel* model)
{
    if (dispatcher)
    {
        dispatcher->commit(create_unique<PasteAssetsCmd>(model, srcs, dest, is_cut, overwrite));
    }
    else
    {
        std::error_code ec;
        for (const auto& src : srcs)
        {
            if (!Filesystem::exists(src))
            {
                continue;
            }
            Path dst = dest / src.filename();
            if (overwrite && Filesystem::exists(dst))
            {
                Filesystem::remove_all(dst, ec);
                ec.clear();
            }
            if (is_cut)
            {
                Filesystem::rename(src, dst, ec);
            }
            else
            {
                Filesystem::copy(
                    src, dst, Filesystem::copy_options::recursive | Filesystem::copy_options::overwrite_existing, ec);
            }
        }
        if (model)
        {
            model->refresh();
        }
    }

    if (is_cut)
    {
        clear();
    }
}

} // namespace Ignis
