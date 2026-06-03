#pragma once

#include <Ignis/Foundation/Foundation.h>
#include "UI/Commands/CommandDispatcher.h"

namespace Ignis
{

class AssetDirectoryModel;

// ---- File operations -------------------------------------------------------

struct RenameAssetCmd : IReversibleCommand
{
    AssetDirectoryModel* model;
    Path                 old_path;
    Path                 new_path;

    RenameAssetCmd(AssetDirectoryModel* m, Path o, Path n)
        : model(m),
          old_path(std::move(o)),
          new_path(std::move(n))
    {
    }

    void execute() override
    {
        apply(old_path, new_path);
    }
    void undo() override
    {
        apply(new_path, old_path);
    }

private:
    void apply(const Path& src, const Path& dst);
};

struct CreateFolderCmd : IReversibleCommand
{
    AssetDirectoryModel* model;
    Path                 folder_path;

    CreateFolderCmd(AssetDirectoryModel* m, Path fp)
        : model(m),
          folder_path(std::move(fp))
    {
    }

    void execute() override;
    void undo() override;
};

struct MoveAssetCmd : IReversibleCommand
{
    AssetDirectoryModel* model;
    Path                 src;
    Path                 dst;

    MoveAssetCmd(AssetDirectoryModel* m, Path s, Path d)
        : model(m),
          src(std::move(s)),
          dst(std::move(d))
    {
    }

    void execute() override
    {
        apply(src, dst);
    }
    void undo() override
    {
        apply(dst, src);
    }

private:
    void apply(const Path& from, const Path& to);
};

// ---- Paste -----------------------------------------------------------------

struct PasteAssetsCmd : IReversibleCommand
{
    AssetDirectoryModel* model;
    Vector<Path>         src_paths; // original source locations (NOT staging paths)
    Path                 dest_dir;
    bool                 is_cut;
    bool                 overwrite;

    PasteAssetsCmd(AssetDirectoryModel* m, Vector<Path> srcs, Path dest, bool cut, bool ovr)
        : model(m),
          src_paths(std::move(srcs)),
          dest_dir(std::move(dest)),
          is_cut(cut),
          overwrite(ovr)
    {
    }

    void execute() override;
    void undo() override;
};

// ---- Delete ----------------------------------------------------------------

// Moves files to .ignis_trash/<stem>_<timestamp><ext> so multiple deletes of
// identically-named files do not collide and undo() can locate each one.
struct DeleteAssetsCmd : IReversibleCommand
{
    AssetDirectoryModel* model;
    Path                 trash_root;

    struct TrashRecord
    {
        Path original;
        Path trashed; // trash_root / stem_timestamp.ext
    };
    Vector<TrashRecord> records;

    DeleteAssetsCmd(AssetDirectoryModel* m, const Vector<Path>& paths, Path trash);

    void execute() override;
    void undo() override;
};

// ---- Tags ------------------------------------------------------------------

struct AddTagCmd : IReversibleCommand
{
    UnorderedMap<String, Vector<String>>* tags;
    String                                path_key;
    String                                tag;

    AddTagCmd(UnorderedMap<String, Vector<String>>* t, String k, String v)
        : tags(t),
          path_key(std::move(k)),
          tag(std::move(v))
    {
    }

    void execute() override
    {
        (*tags)[path_key].push_back(tag);
    }
    void undo() override
    {
        auto& v = (*tags)[path_key];
        v.erase(std::find(v.begin(), v.end(), tag));
    }
};

struct RemoveTagCmd : IReversibleCommand
{
    UnorderedMap<String, Vector<String>>* tags;
    String                                path_key;
    String                                tag;

    RemoveTagCmd(UnorderedMap<String, Vector<String>>* t, String k, String v)
        : tags(t),
          path_key(std::move(k)),
          tag(std::move(v))
    {
    }

    void execute() override
    {
        auto& v = (*tags)[path_key];
        v.erase(std::find(v.begin(), v.end(), tag));
    }
    void undo() override
    {
        (*tags)[path_key].push_back(tag);
    }
};

// ---- Favorites -------------------------------------------------------------

struct PinFavoriteCmd : IReversibleCommand
{
    Vector<Path>* favorites;
    Path          path;

    PinFavoriteCmd(Vector<Path>* f, Path p)
        : favorites(f),
          path(std::move(p))
    {
    }

    void execute() override
    {
        favorites->push_back(path);
    }
    void undo() override
    {
        favorites->erase(std::find(favorites->begin(), favorites->end(), path));
    }
};

struct UnpinFavoriteCmd : IReversibleCommand
{
    Vector<Path>* favorites;
    Path          path;

    UnpinFavoriteCmd(Vector<Path>* f, Path p)
        : favorites(f),
          path(std::move(p))
    {
    }

    void execute() override
    {
        favorites->erase(std::find(favorites->begin(), favorites->end(), path));
    }
    void undo() override
    {
        favorites->push_back(path);
    }
};

} // namespace Ignis
