#pragma once
#include "Ignis/Foundation/Foundation.h"

namespace Ignis
{

enum class FileDialogMode
{
    OpenProject,
    OpenScene,
    ImportAsset
};

struct FileDialogOptions
{
    Optional<Path>           initial_dir;
    Optional<Vector<String>> filters; // extension strings without dot, e.g. {"png","jpg"}; NullOpt = mode defaults
};

class FileDialog
{
public:
    static Optional<Path> open(FileDialogMode mode, const FileDialogOptions& opts = {});
    static Optional<Path> save_scene(const FileDialogOptions& opts = {});
};

} // namespace Ignis
