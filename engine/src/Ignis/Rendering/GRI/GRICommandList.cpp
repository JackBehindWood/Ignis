#include "igpch.h"
#include "GRICommandList.h"

#include <Ignis/Rendering/RenderSystem.h>
#include "GRIContext.h"

#include "GRICommandListCommandExecute.inl"

namespace Ignis
{
	class GRICommandListIterator
	{
	private:
		GRICommandBase* m_cmd_ptr;
	public:
		GRICommandListIterator(GRICommandListBase& cmd_list) : m_cmd_ptr(cmd_list.m_root) {}

		inline bool has_commands_left() const { return !!m_cmd_ptr; }
		inline GRICommandBase* next_command()
		{
			GRICommandBase* cmd = m_cmd_ptr;
			m_cmd_ptr = cmd->next;
			return cmd;
		}
	};

	void GRICommandListBase::execute()
	{
		IG_CORE_ASSERT(!is_executing(), "Command List is already executing");
		m_executing = true;

		IG_CORE_INFO("{0} commands in command list", m_num_commands);

		GRICommandListIterator iter(*this);
		while (iter.has_commands_left())
		{
			GRICommandBase* cmd = iter.next_command();
			cmd->execute_and_destruct(*this);
		}
	}

	GRICommandListBase::GRICommandListBase() : m_root(nullptr), m_link(&m_root), m_num_commands(0), m_context(nullptr), m_executing(false)
	{
	}

	GRICommandListBase::~GRICommandListBase()
	{
	}

	void GRICommandListExecutor::submit()
	{
		GRICommandListBase* cmd_list = nullptr;
		{
			cmd_list = new GRICommandListBase(std::move(static_cast<GRICommandListBase&>(m_command_list)));

			GRICommandContext* context = static_cast<GRICommandContext*>(m_command_list.m_context);

			static_cast<GRICommandListBase&>(m_command_list).~GRICommandListBase();
			new (&m_command_list) GRICommandListBase();

			m_command_list.m_context = context;
		}

		cmd_list->execute();

		delete cmd_list;
	}
}