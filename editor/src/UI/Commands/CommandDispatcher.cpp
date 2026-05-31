#include "edpch.h"
#include "CommandDispatcher.h"

namespace Ignis
{

CommandDispatcher* CommandDispatcher::s_active = nullptr;

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

void CommandDispatcher::commit(UniquePtr<IReversibleCommand> cmd)
{
    cmd->execute();
    if (m_undo.size() >= k_history_max)
    {
        m_undo.pop_front();
    }
    m_undo.push_back(std::move(cmd));
    m_redo.clear();
}

void CommandDispatcher::undo()
{
    if (m_undo.empty())
    {
        return;
    }
    m_undo.back()->undo();
    m_redo.push_back(std::move(m_undo.back()));
    m_undo.pop_back();
}

void CommandDispatcher::redo()
{
    if (m_redo.empty())
    {
        return;
    }
    m_redo.back()->redo();
    m_undo.push_back(std::move(m_redo.back()));
    m_redo.pop_back();
}

void CommandDispatcher::clear_history()
{
    m_undo.clear();
    m_redo.clear();
}

} // namespace Ignis
