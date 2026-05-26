#pragma once

#include <Ignis.h>
#include "ProjectDescriptor.h"
#include "IProjectObserver.h"

namespace Ignis
{

enum class OpenResult
{
    Ok,
    FileNotFound,
    ParseError,
    MissingDirectory,
    ReadOnlyDirectory,
    AlreadyOpen
};

enum class ProjectOpenMode
{
    InMemory,
    FromDisk
};

class ProjectManager
{
public:
    static ProjectManager& get();

    OpenResult open(const Path& igproject_file);
    OpenResult open_in_memory(const ProjectDescriptor& desc);
    void       close();
    void       save();
    OpenResult create(const Path& directory, const String& name);

    const ProjectDescriptor& descriptor() const
    {
        return m_descriptor;
    }
    bool is_open() const
    {
        return m_open;
    }
    bool is_dirty() const
    {
        return m_dirty.load();
    }
    bool is_in_memory() const
    {
        return m_in_memory;
    }

    struct ProjectPaths
    {
        Path asset_source;
        Path compiled_cache;
    };
    Optional<ProjectPaths> snapshot_paths() const;

    void add_observer(IProjectObserver* obs);
    void remove_observer(IProjectObserver* obs);

private:
    OpenResult validate(ProjectOpenMode mode);
    void       dispatch_project_opened();
    void       mark_dirty()
    {
        m_dirty.store(true);
    }
    void write_lock_file();
    void remove_lock_file();

    ProjectDescriptor         m_descriptor;
    Vector<IProjectObserver*> m_observers;
    bool                      m_open      = false;
    bool                      m_in_memory = false;
    RelaxedAtomic<bool>       m_dirty;
    mutable SharedMutex       m_paths_mutex;
    ProjectPaths              m_cached_paths;
    Path                      m_lock_file_path;
};

} // namespace Ignis
