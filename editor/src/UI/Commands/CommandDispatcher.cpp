#include "edpch.h"
#include "CommandDispatcher.h"

namespace Ignis
{

void CommandDispatcher::enqueue(UniquePtr<IEditorCommand> cmd)
{
    m_queue.push_back(std::move(cmd));
}

void CommandDispatcher::flush(WorkspaceManager& mgr)
{
    for (auto& cmd : m_queue)
    {
        cmd->execute(mgr);
    }
    m_queue.clear();
}

} // namespace Ignis
