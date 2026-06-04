#include "Ignis/Core/FileDialog.h"

#ifdef IG_PLATFORM_WINDOWS
#include <windows.h>
#include <shobjidl.h>
#include <combaseapi.h>

namespace Ignis
{

namespace
{
struct FilterSpec
{
    const wchar_t* name;
    const wchar_t* spec;
};

static const FilterSpec k_filters[][4] = {
    // OpenProject
    {{L"Ignis Project", L"*.igproject"}, {nullptr, nullptr}},
    // OpenScene
    {{L"Ignis Scene", L"*.igscene"}, {nullptr, nullptr}},
    // ImportAsset
    {{L"Images", L"*.png;*.jpg;*.jpeg"},
     {L"Meshes", L"*.obj;*.fbx"},
     {L"Materials", L"*.mtl;*.igmat"},
     {nullptr, nullptr}},
};
} // namespace

Optional<Path> FileDialog::open(FileDialogMode mode, const FileDialogOptions& opts)
{
    // S_FALSE = already initialized on this thread; do not call CoUninitialize in that case.
    const HRESULT co_hr    = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool    co_owned = (co_hr == S_OK);
    if (FAILED(co_hr) && co_hr != S_FALSE)
    {
        return NullOpt;
    }

    IFileOpenDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog,
                                reinterpret_cast<void**>(&dialog))))
    {
        if (co_owned)
        {
            CoUninitialize();
        }
        return NullOpt;
    }

    if (opts.filters)
    {
        Vector<std::wstring>      specs_storage;
        Vector<COMDLG_FILTERSPEC> filter_specs;
        std::wstring              all_specs;
        for (const String& ext : *opts.filters)
        {
            all_specs += L"*." + std::wstring(ext.begin(), ext.end()) + L";";
        }
        if (!all_specs.empty())
        {
            all_specs.pop_back();
        }
        specs_storage.push_back(std::move(all_specs));

        filter_specs.push_back({L"Files", specs_storage.back().c_str()});
        dialog->SetFileTypes(static_cast<UINT>(filter_specs.size()), filter_specs.data());
    }
    else
    {
        const FilterSpec* specs      = k_filters[static_cast<int>(mode)];
        int               spec_count = 0;
        while (specs[spec_count].name)
        {
            ++spec_count;
        }

        Vector<COMDLG_FILTERSPEC> filter_specs(static_cast<size_t>(spec_count));
        for (int i = 0; i < spec_count; ++i)
        {
            filter_specs[i].pszName = specs[i].name;
            filter_specs[i].pszSpec = specs[i].spec;
        }
        dialog->SetFileTypes(static_cast<UINT>(spec_count), filter_specs.data());
    }

    if (opts.initial_dir)
    {
        std::wstring wpath(opts.initial_dir->wstring());
        IShellItem*  folder = nullptr;
        if (SUCCEEDED(
                SHCreateItemFromParsingName(wpath.c_str(), nullptr, IID_IShellItem, reinterpret_cast<void**>(&folder))))
        {
            dialog->SetDefaultFolder(folder);
            folder->Release();
        }
    }

    Optional<Path> result = NullOpt;
    if (SUCCEEDED(dialog->Show(nullptr)))
    {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item)))
        {
            PWSTR path_str = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path_str)))
            {
                result = Path{path_str};
                CoTaskMemFree(path_str);
            }
            item->Release();
        }
    }

    dialog->Release();
    if (co_owned)
    {
        CoUninitialize();
    }
    return result;
}

} // namespace Ignis
#endif // IG_PLATFORM_WINDOWS
