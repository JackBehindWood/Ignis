#include "ProjectManager.h"
#include "ProjectSerializer.h"
#include "Ignis/Core/Platform.h"

namespace Ignis
{

ProjectManager& ProjectManager::get()
{
    static ProjectManager instance;
    return instance;
}

void ProjectManager::add_observer(IProjectObserver* obs)
{
    m_observers.push_back(obs);
}

void ProjectManager::remove_observer(IProjectObserver* obs)
{
    m_observers.erase(std::remove(m_observers.begin(), m_observers.end(), obs), m_observers.end());
}

void ProjectManager::dispatch_project_opened()
{
    const Path asset_source   = m_in_memory ? Path{} : (m_descriptor.root / m_descriptor.asset_source_dir);
    const Path compiled_cache = m_in_memory ? Path{} : (m_descriptor.root / m_descriptor.compiled_cache_dir);

    {
        UniqueLock lk(m_paths_mutex);
        m_cached_paths = {asset_source, compiled_cache};
    }

    const ProjectContext ctx{m_descriptor, asset_source, compiled_cache, m_in_memory};

    for (auto* obs : m_observers)
    {
        obs->on_project_opened(ctx);
    }
}

void ProjectManager::write_lock_file()
{
    Ofstream f(m_lock_file_path);
    if (f.is_open())
    {
        f << "pid:" << Platform::current_pid() << "\n";
        f << "timestamp:" << Platform::current_time_s() << "\n";
    }
}

void ProjectManager::remove_lock_file()
{
    if (!m_lock_file_path.empty())
    {
        Filesystem::remove(m_lock_file_path);
    }
}

OpenResult ProjectManager::open_in_memory(const ProjectDescriptor& desc)
{
    if (m_open)
    {
        close();
    }

    m_descriptor = desc;
    m_in_memory  = true;

    if (const OpenResult r = validate(ProjectOpenMode::InMemory); r != OpenResult::Ok)
    {
        return r;
    }

    dispatch_project_opened();
    m_open = true;

    IG_INFO("ProjectManager: opened in-memory project '{0}'", desc.name);
    return OpenResult::Ok;
}

OpenResult ProjectManager::open(const Path& igproject_file)
{
    if (!Filesystem::exists(igproject_file))
    {
        IG_ERROR("ProjectManager: project file not found: {0}", igproject_file.string());
        return OpenResult::FileNotFound;
    }

    const Path lock_path = igproject_file.parent_path() / (igproject_file.stem().string() + ".igproject.lock");
    if (Filesystem::exists(lock_path))
    {
        int64_t pid            = 0;
        int64_t lock_timestamp = 0;
        {
            Ifstream lf(lock_path);
            String   line;
            while (std::getline(lf, line))
            {
                if (line.compare(0, 4, "pid:") == 0)
                {
                    pid = std::stoll(line.substr(4));
                }
                else if (line.compare(0, 10, "timestamp:") == 0)
                {
                    lock_timestamp = std::stoll(line.substr(10));
                }
            }
            if (pid == 0)
            {
                Stringstream ss(line);
                ss >> pid;
            }
        }
        const bool predates_boot = (lock_timestamp > 0) && (lock_timestamp < Platform::boot_time_epoch_s());
        const bool alive         = !predates_boot && Platform::is_process_alive(pid);
        if (alive)
        {
            IG_WARN("ProjectManager: project already open by PID {0}", pid);
            return OpenResult::AlreadyOpen;
        }
        IG_WARN("ProjectManager: stale lock file from PID {0} — overriding", pid);
        Filesystem::remove(lock_path);
    }

    ProjectDescriptor desc;
    if (!ProjectSerializer::read(igproject_file, desc))
    {
        return OpenResult::ParseError;
    }

    m_descriptor = desc;
    m_in_memory  = false;

    if (const OpenResult r = validate(ProjectOpenMode::FromDisk); r != OpenResult::Ok)
    {
        return r;
    }

    m_lock_file_path = lock_path;
    write_lock_file();

    dispatch_project_opened();
    m_open = true;

    IG_INFO("ProjectManager: opened '{0}'", desc.name);
    return OpenResult::Ok;
}

void ProjectManager::close()
{
    if (!m_open)
    {
        return;
    }

    for (auto* obs : m_observers)
    {
        obs->on_project_closed();
    }

    if (!m_in_memory)
    {
        save();
    }

    {
        UniqueLock lk(m_paths_mutex);
        m_cached_paths = {};
    }
    remove_lock_file();
    m_lock_file_path.clear();
    m_descriptor = {};
    m_dirty.store(false);
    m_open      = false;
    m_in_memory = false;
    IG_INFO("ProjectManager: closed");
}

void ProjectManager::save()
{
    if (!m_open || m_in_memory)
    {
        return;
    }

    const Path proj_file = m_descriptor.root / (m_descriptor.name + ".igproject");
    ProjectSerializer::write(proj_file, m_descriptor);

    m_dirty.store(false);
}

OpenResult ProjectManager::create(const Path& directory, const String& name)
{
    const Path root = directory / name;

    std::error_code ec;
    Filesystem::create_directories(root / "assets", ec);
    if (ec)
    {
        IG_ERROR("ProjectManager: cannot create project directory: {0}", ec.message());
        return OpenResult::MissingDirectory;
    }
    Filesystem::create_directories(root / "cache", ec);
    if (ec)
    {
        IG_ERROR("ProjectManager: cannot create cache directory: {0}", ec.message());
        return OpenResult::MissingDirectory;
    }

    ProjectDescriptor desc;
    desc.name               = name;
    desc.root               = root;
    desc.asset_source_dir   = "assets";
    desc.compiled_cache_dir = "cache";

    const Path proj_file = root / (name + ".igproject");
    if (!ProjectSerializer::write(proj_file, desc))
    {
        return OpenResult::ParseError;
    }

    return open(proj_file);
}

OpenResult ProjectManager::validate(ProjectOpenMode mode)
{
    if (mode == ProjectOpenMode::InMemory)
    {
        if (m_descriptor.name.empty())
        {
            IG_ERROR("ProjectManager: in-memory project must have a non-empty name");
            return OpenResult::ParseError;
        }
        return OpenResult::Ok;
    }

    auto check_dir = [&](const Path& p) -> bool
    {
        if (!Filesystem::exists(p) || !Filesystem::is_directory(p))
        {
            IG_ERROR("ProjectManager: required directory missing: {0}", p.string());
            return false;
        }
        return true;
    };

    const Path& root = m_descriptor.root;
    if (!check_dir(root))
    {
        return OpenResult::MissingDirectory;
    }
    if (!check_dir(root / m_descriptor.asset_source_dir))
    {
        return OpenResult::MissingDirectory;
    }
    if (!check_dir(root / m_descriptor.compiled_cache_dir))
    {
        return OpenResult::MissingDirectory;
    }

    const Path probe = root / m_descriptor.compiled_cache_dir / ".write_probe";
    {
        Ofstream f(probe);
        if (!f.is_open())
        {
            IG_ERROR("ProjectManager: cache directory is read-only: {0}",
                     (root / m_descriptor.compiled_cache_dir).string());
            return OpenResult::ReadOnlyDirectory;
        }
    }
    Filesystem::remove(probe);

    return OpenResult::Ok;
}

Optional<ProjectManager::ProjectPaths> ProjectManager::snapshot_paths() const
{
    SharedLock lk(m_paths_mutex);
    if (!m_open)
    {
        return NullOpt;
    }
    return m_cached_paths;
}

} // namespace Ignis
