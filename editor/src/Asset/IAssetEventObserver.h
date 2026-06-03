#pragma once

#include <Ignis/Foundation/Foundation.h>

namespace Ignis
{

struct IAssetEventObserver
{
    virtual void on_asset_imported(const Path& abs_path)                      = 0;
    virtual void on_asset_renamed(const Path& old_path, const Path& new_path) = 0;
    virtual void on_asset_deleted(const Path& abs_path)                       = 0;
    virtual void on_directory_changed()                                       = 0;

protected:
    ~IAssetEventObserver() = default;
};

} // namespace Ignis
