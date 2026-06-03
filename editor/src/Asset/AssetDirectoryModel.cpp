#include "edpch.h"
#include "AssetDirectoryModel.h"
#include "Asset/EditorAssetManager.h"

namespace Ignis
{

namespace
{

bool is_asset_missing(const Path& p)
{
    return !Filesystem::exists(p);
}

String derive_type_label(const Path& path, bool is_dir)
{
    if (is_dir)
    {
        return "DIR";
    }

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

void AssetDirectoryModel::init(const Path& assets_root)
{
    m_root = assets_root;
    EditorAssetManager::get().add_observer(this);
    navigate_raw(assets_root);
}

void AssetDirectoryModel::shutdown()
{
    EditorAssetManager::get().remove_observer(this);
    m_root.clear();
    m_current_dir.clear();
    m_entries.clear();
    m_history_back.clear();
    m_history_forward.clear();
}

void AssetDirectoryModel::tick(float ts)
{
    if (m_needs_refresh)
    {
        m_needs_refresh = false;
        rebuild_entries();
        return;
    }

    if (m_current_dir.empty())
    {
        return;
    }

    m_poll_timer += ts;
    if (m_poll_timer >= 2.0f)
    {
        m_poll_timer = 0.0f;
        std::error_code ec;
        auto            mtime = Filesystem::last_write_time(m_current_dir, ec);
        if (!ec && mtime != m_dir_mtime)
        {
            m_dir_mtime = mtime;
            rebuild_entries();
        }
    }
}

void AssetDirectoryModel::navigate_to(const Path& dir)
{
    if (!m_current_dir.empty() && m_current_dir != dir)
    {
        if (m_history_back.empty() || m_history_back.back() != m_current_dir)
        {
            m_history_back.push_back(m_current_dir);
            if (m_history_back.size() > 50)
            {
                m_history_back.pop_front();
            }
        }
    }
    m_history_forward.clear();
    navigate_raw(dir);
}

void AssetDirectoryModel::navigate_back()
{
    if (m_history_back.empty())
    {
        return;
    }
    m_history_forward.push_back(m_current_dir);
    Path target = m_history_back.back();
    m_history_back.pop_back();
    navigate_raw(target);
}

void AssetDirectoryModel::navigate_forward()
{
    if (m_history_forward.empty())
    {
        return;
    }
    m_history_back.push_back(m_current_dir);
    Path target = m_history_forward.back();
    m_history_forward.pop_back();
    navigate_raw(target);
}

void AssetDirectoryModel::refresh()
{
    m_needs_refresh = true;
}

void AssetDirectoryModel::set_search(const char* text)
{
    std::strncpy(m_search_buf, text, sizeof(m_search_buf) - 1);
    m_search_buf[sizeof(m_search_buf) - 1] = '\0';
    rebuild_entries();
}

void AssetDirectoryModel::set_filter_type(int32_t type)
{
    m_filter_type = type;
    rebuild_entries();
}

void AssetDirectoryModel::set_filter_tag(const char* tag)
{
    std::strncpy(m_filter_tag, tag, sizeof(m_filter_tag) - 1);
    m_filter_tag[sizeof(m_filter_tag) - 1] = '\0';
    rebuild_entries();
}

void AssetDirectoryModel::navigate_raw(const Path& dir)
{
    m_current_dir = dir;

    std::error_code ec;
    m_dir_mtime  = Filesystem::last_write_time(m_current_dir, ec);
    m_poll_timer = 0.0f;
    if (ec)
    {
        m_dir_mtime = {};
    }

    rebuild_entries();
}

void AssetDirectoryModel::rebuild_entries()
{
    m_entries.clear();

    if (m_current_dir.empty() || !Filesystem::exists(m_current_dir))
    {
        return;
    }

    std::error_code ec;
    bool            is_searching = (m_search_buf[0] != '\0');

    auto add_entry = [&](const Filesystem::directory_entry& fs_entry)
    {
        bool      is_dir = fs_entry.is_directory();
        uintmax_t size   = is_dir ? 0 : fs_entry.file_size(ec);
        if (ec)
        {
            size = 0;
            ec.clear();
        }

        Entry e;
        e.path           = fs_entry.path();
        e.is_dir         = is_dir;
        e.display_name   = fs_entry.path().filename().string();
        e.type_label     = derive_type_label(fs_entry.path(), is_dir);
        e.file_size      = size;
        e.status_missing = is_asset_missing(e.path);
        m_entries.push_back(std::move(e));
    };

    if (is_searching)
    {
        for (const auto& entry : Filesystem::recursive_directory_iterator(m_current_dir, ec))
        {
            if (ec)
            {
                ec.clear();
                break;
            }
            String filename = entry.path().filename().string();
            if (str_icontains(filename.c_str(), m_search_buf))
            {
                add_entry(entry);
            }
        }
    }
    else
    {
        for (const auto& entry : Filesystem::directory_iterator(m_current_dir, ec))
        {
            if (ec)
            {
                break;
            }
            add_entry(entry);
        }
    }

    // Type + tag filtering
    if (m_filter_type != 0 || m_filter_tag[0] != '\0')
    {
        auto it = std::remove_if(m_entries.begin(), m_entries.end(),
                                 [this](const Entry& e)
                                 {
                                     if (m_filter_type == 1 && !e.is_dir)
                                     {
                                         return true;
                                     }
                                     if (m_filter_type == 2 && e.type_label != "TEX")
                                     {
                                         return true;
                                     }
                                     if (m_filter_type == 3 && e.type_label != "SHD")
                                     {
                                         return true;
                                     }
                                     if (m_filter_type == 4 && e.type_label != "MSH")
                                     {
                                         return true;
                                     }
                                     if (m_filter_type == 5 && e.type_label != "SCN")
                                     {
                                         return true;
                                     }
                                     return false;
                                 });
        m_entries.erase(it, m_entries.end());
    }

    std::sort(m_entries.begin(), m_entries.end(),
              [](const Entry& a, const Entry& b)
              {
                  if (a.is_dir != b.is_dir)
                  {
                      return a.is_dir > b.is_dir;
                  }
                  String na = a.display_name;
                  String nb = b.display_name;
                  std::transform(na.begin(), na.end(), na.begin(), [](unsigned char c) { return std::tolower(c); });
                  std::transform(nb.begin(), nb.end(), nb.begin(), [](unsigned char c) { return std::tolower(c); });
                  return na < nb;
              });
}

} // namespace Ignis
