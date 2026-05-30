#pragma once

namespace Ignis
{

class WorkspaceManager;

struct IEditorCommand
{
    virtual ~IEditorCommand()                   = default;
    virtual void execute(WorkspaceManager& mgr) = 0;
};

class CommandDispatcher
{
public:
    void enqueue(UniquePtr<IEditorCommand> cmd);
    void flush(WorkspaceManager& mgr);

private:
    Vector<UniquePtr<IEditorCommand>> m_queue;
};

} // namespace Ignis
