#include "edpch.h"
#include "AssetBrowserCommands.h"
#include "AssetDirectoryModel.h"

#include <chrono>

namespace Ignis
{

// ---- RenameAssetCmd --------------------------------------------------------

void RenameAssetCmd::apply(const Path& src, const Path& dst)
{
    std::error_code ec;
    Filesystem::rename(src, dst, ec);
    model->refresh();
}

// ---- CreateFolderCmd -------------------------------------------------------

void CreateFolderCmd::execute()
{
    std::error_code ec;
    Filesystem::create_directory(folder_path, ec);
    model->refresh();
}

void CreateFolderCmd::undo()
{
    std::error_code ec;
    Filesystem::remove(folder_path, ec);
    model->refresh();
}

// ---- MoveAssetCmd ----------------------------------------------------------

void MoveAssetCmd::apply(const Path& from, const Path& to)
{
    std::error_code ec;
    Filesystem::rename(from, to, ec);
    model->refresh();
}

// ---- PasteAssetsCmd --------------------------------------------------------

void PasteAssetsCmd::execute()
{
    std::error_code ec;
    for (const auto& src : src_paths)
    {
        if (!Filesystem::exists(src))
        {
            continue;
        }

        Path dst = dest_dir / src.filename();

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
            Filesystem::copy(src, dst,
                             Filesystem::copy_options::recursive | Filesystem::copy_options::overwrite_existing, ec);
        }
    }
    model->refresh();
}

void PasteAssetsCmd::undo()
{
    std::error_code ec;
    for (const auto& src : src_paths)
    {
        Path dst = dest_dir / src.filename();
        if (is_cut)
        {
            Filesystem::rename(dst, src, ec); // restore to original location
        }
        else
        {
            Filesystem::remove_all(dst, ec);
        }
    }
    model->refresh();
}

// ---- DeleteAssetsCmd -------------------------------------------------------

static String make_trash_name(const Path& p)
{
    auto   now  = std::chrono::steady_clock::now().time_since_epoch();
    auto   us   = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
    String stem = p.stem().string();
    String ext  = p.extension().string();
    return stem + "_" + std::to_string(us) + ext;
}

DeleteAssetsCmd::DeleteAssetsCmd(AssetDirectoryModel* m, const Vector<Path>& paths, Path trash)
    : model(m),
      trash_root(std::move(trash))
{
    records.reserve(paths.size());
    for (const auto& p : paths)
    {
        records.push_back({p, trash_root / make_trash_name(p)});
    }
}

void DeleteAssetsCmd::execute()
{
    std::error_code ec;
    Filesystem::create_directories(trash_root, ec);
    for (auto& rec : records)
    {
        if (Filesystem::exists(rec.original))
        {
            Filesystem::rename(rec.original, rec.trashed, ec);
        }
    }
    model->refresh();
}

void DeleteAssetsCmd::undo()
{
    std::error_code ec;
    for (auto& rec : records)
    {
        if (Filesystem::exists(rec.trashed))
        {
            Filesystem::rename(rec.trashed, rec.original, ec);
        }
    }
    model->refresh();
}

} // namespace Ignis
