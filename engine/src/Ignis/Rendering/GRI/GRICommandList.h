#pragma once

#include <Ignis/Foundation/MemStack.h>
#include "GRIDefinitions.h"

namespace Ignis
{

	struct GRICommandBase
	{
		GRICommandBase* next = nullptr;
		virtual void execute_and_destruct(GRICommandListBase& cmd_list) = 0;
	};

	template<typename TCmd>
	struct GRICommand : public GRICommandBase
	{
		void execute_and_destruct(GRICommandListBase& cmd_list) override final
		{
			TCmd* cmd = static_cast<TCmd*>(this);
			cmd->execute(cmd_list);
			cmd->~TCmd();
		}
	};

	template <typename GRICmdListType, typename LAMBDA>
	struct GRILambdaCommand final : public GRICommandBase
	{
		LAMBDA lambda;

		GRILambdaCommand(LAMBDA&& lambda, const char* name)
			: lambda(std::forward<LAMBDA>(lambda)) {}

		void execute_and_destruct(GRICommandListBase& cmd_list) override final
		{
			lambda(*static_cast<GRICmdListType*>(&cmd_list));
			lambda.~LAMBDA();
		}
	};

	#define ALLOC_COMMAND(...) new ( alloc_command(sizeof(__VA_ARGS__), alignof(__VA_ARGS__)) ) __VA_ARGS__
	#define GRICOMMAND_MACRO(CommandName)							\
	struct CommandName final : public GRICommand<CommandName>

	class GRICommandListBase
	{
		friend class GRICommandListIterator;
		friend class GRICommandListExecutor;
        friend class RenderSystem;
	private:
		MemStack m_mem_stack;
		uint32_t m_num_commands;
		GRICommandBase* m_root;
		GRICommandBase** m_link;

		GRICommandContext* m_context;

		bool m_executing;

		void execute();
        
        inline void initialise_context(GRICommandContext* context) { m_context = context; }
	public:
		GRICommandListBase();
		~GRICommandListBase();

		// Move only.
		GRICommandListBase(const GRICommandListBase&) = delete;
		GRICommandListBase(GRICommandListBase&& other) = default;

		inline void* alloc(int64_t alloc_size, int64_t alignment)
		{
			return m_mem_stack.allocate(alloc_size, alignment);
		}

		template <typename T>
		inline T* alloc()
		{
			return (T*)alloc(sizeof(T), alignof(T));
		}

		inline void* alloc_copy(const void* data, int64_t alloc_size, int64_t alignment)
		{
			void* new_data = alloc(alloc_size, alignment);
			memcpy(new_data, data, alloc_size);
			return new_data;
		}

		inline void* alloc_command(int32_t alloc_size, int32_t alignment)
		{
			IG_CORE_ASSERT(!is_executing(), "Cannot allocate commands when running");
			GRICommandBase* result = (GRICommandBase*)m_mem_stack.allocate(alloc_size, alignment);
			++m_num_commands;
			*m_link = result;
			m_link = &result->next;
			return result;
		}

		template <typename Tcmd>
		inline void* alloc_command()
		{
			return alloc_command(sizeof(Tcmd), alignof(Tcmd));
		}

		template <typename LAMBDA>
		inline void enqueue_lambda(LAMBDA&& lambda)
		{
			ALLOC_COMMAND(GRILambdaCommand<GRICommandListBase, LAMBDA>)(std::forward<LAMBDA>(lambda));
		}

		inline GRICommandContext& get_context() { return *m_context; }

		const uint32_t get_used_memory() const { return (uint32_t)m_mem_stack.get_total_allocated(); }
		inline bool is_executing() const { return m_executing; }
		inline bool has_commands() const { return !m_mem_stack.empty(); }
	};

	GRICOMMAND_MACRO(GRICommandBeginDrawingViewport)
	{
		GRIViewport* viewport;
		GRITexture2D* render_target;
		GRICommandBeginDrawingViewport(GRIViewport* viewport, GRITexture2D* render_target) : viewport(viewport), render_target(render_target) {}

		void execute(GRICommandListBase & cmd_list);
	};

	GRICOMMAND_MACRO(GRICommandEndDrawingViewport)
	{
		GRIViewport* viewport;
		GRICommandEndDrawingViewport(GRIViewport * viewport) : viewport(viewport) {}

		void execute(GRICommandListBase & cmd_list);
	};

	class GRICommandList : public GRICommandListBase
	{
	private:
	public:
		GRICommandList() : GRICommandListBase() {}

		GRICommandList(GRICommandListBase&& other)
			: GRICommandListBase(std::move(other))
		{
		}

		static inline GRICommandList& get(GRICommandListBase& cmd_list)
		{
			return static_cast<GRICommandList&>(cmd_list);
		}

		template <typename LAMBDA>
		inline void enqueue_lambda(LAMBDA&& lambda)
		{
			ALLOC_COMMAND(GRILambdaCommand<GRICommandList, LAMBDA>)(std::forward<LAMBDA>(lambda));
		}

		inline void begin_drawing_viewport(GRIViewport* viewport, GRITexture2D* render_target)
		{
			ALLOC_COMMAND(GRICommandBeginDrawingViewport)(viewport, render_target);
		}

		inline void end_drawing_viewport(GRIViewport* viewport)
		{
			ALLOC_COMMAND(GRICommandEndDrawingViewport)(viewport);
		}
	};



	class GRICommandListExecutor
	{
	private:
		GRICommandList m_command_list;
	public:

		inline GRICommandList& get_command_list() { return m_command_list; }

		void submit();
	};
}