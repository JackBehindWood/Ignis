#pragma once

namespace Ignis
{

class WorkspaceManager;

struct IEditorCommand
{
    virtual ~IEditorCommand()                   = default;
    virtual void execute(WorkspaceManager& mgr) = 0;
};

struct IReversibleCommand
{
    virtual ~IReversibleCommand() = default;
    virtual void execute()        = 0;
    virtual void undo()           = 0;
    virtual void redo()
    {
        execute();
    }
};

class CommandDispatcher
{
public:
    static constexpr uint32_t k_history_max = 100;

    void enqueue(UniquePtr<IEditorCommand> cmd);
    void flush(WorkspaceManager& mgr);

    void commit(UniquePtr<IReversibleCommand> cmd);
    void undo();
    void redo();
    bool can_undo() const
    {
        return !m_undo.empty();
    }
    bool can_redo() const
    {
        return !m_redo.empty();
    }
    void clear_history();

    static void set_active(CommandDispatcher* d)
    {
        s_active = d;
    }
    static CommandDispatcher* active()
    {
        return s_active;
    }

private:
    static CommandDispatcher* s_active;

    Vector<UniquePtr<IEditorCommand>>    m_queue;
    Deque<UniquePtr<IReversibleCommand>> m_undo;
    Deque<UniquePtr<IReversibleCommand>> m_redo;
};

} // namespace Ignis
