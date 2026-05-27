#pragma once

#include <Ignis/Foundation/MemStack.h>
#include "GRIDefinitions.h"
#include "GRIResource.h"

namespace Ignis
{

struct GRICommandBase
{
    GRICommandBase* next                                               = nullptr;
    virtual void    execute_and_destruct(GRICommandListBase& cmd_list) = 0;
};

template <typename TCmd>
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
        : lambda(std::forward<LAMBDA>(lambda))
    {
    }

    void execute_and_destruct(GRICommandListBase& cmd_list) override final
    {
        lambda(*static_cast<GRICmdListType*>(&cmd_list));
        lambda.~LAMBDA();
    }
};

#define ALLOC_COMMAND(...) new (alloc_command(sizeof(__VA_ARGS__), alignof(__VA_ARGS__))) __VA_ARGS__
#define GRICOMMAND_MACRO(CommandName) struct CommandName final : public GRICommand<CommandName>

class GRICommandListBase
{
    friend class GRICommandListIterator;
    friend class GRICommandListExecutor;
    friend class RenderSystem;

private:
    MemStack         m_mem_stack;
    uint32_t         m_num_commands;
    GRICommandBase*  m_root;
    GRICommandBase** m_link;

    GRICommandContext* m_context;

    bool m_executing;

    void execute();

    inline void initialise_context(GRICommandContext* context)
    {
        m_context = context;
    }

public:
    GRICommandListBase();
    ~GRICommandListBase();

    // Move only.
    GRICommandListBase(const GRICommandListBase&)  = delete;
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
        m_link  = &result->next;
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

    inline GRICommandContext& get_context()
    {
        return *m_context;
    }

    const uint32_t get_used_memory() const
    {
        return (uint32_t)m_mem_stack.get_total_allocated();
    }
    inline bool is_executing() const
    {
        return m_executing;
    }
    inline bool has_commands() const
    {
        return !m_mem_stack.empty();
    }
};

GRICOMMAND_MACRO(GRICommandBeginDrawingViewport)
{
    GRIViewport*  viewport;
    GRITexture2D* render_target;
    GRICommandBeginDrawingViewport(GRIViewport * viewport, GRITexture2D * render_target)
        : viewport(viewport),
          render_target(render_target)
    {
    }

    void execute(GRICommandListBase & cmd_list);
};

GRICOMMAND_MACRO(GRICommandBeginFrame)
{
    GRICommandBeginFrame()
    {
    }

    void execute(GRICommandListBase & cmd_list);
};

GRICOMMAND_MACRO(GRICommandEndFrame)
{
    GRICommandEndFrame()
    {
    }

    void execute(GRICommandListBase & cmd_list);
};

GRICOMMAND_MACRO(GRICommandBeginRenderPass)
{
    GRIRenderPassInfo info;
    GRICommandBeginRenderPass(const GRIRenderPassInfo& info)
        : info(info)
    {
    }
    void execute(GRICommandListBase & cmd_list);
};

GRICOMMAND_MACRO(GRICommandEndRenderPass)
{
    GRICommandEndRenderPass()
    {
    }
    void execute(GRICommandListBase & cmd_list);
};

GRICOMMAND_MACRO(GRICommandSetVertexBuffer)
{
    GRIBuffer* buffer;
    uint32_t   offset;
    uint32_t   buffer_index;
    GRICommandSetVertexBuffer(GRIBuffer * buffer, uint32_t offset, uint32_t buffer_index)
        : buffer(buffer),
          offset(offset),
          buffer_index(buffer_index)
    {
    }

    void execute(GRICommandListBase & cmd_list);
};

GRICOMMAND_MACRO(GRICommandSetIndexBuffer)
{
    GRIBuffer*     buffer;
    GRIIndexFormat format;
    uint32_t       offset;
    GRICommandSetIndexBuffer(GRIBuffer * buffer, GRIIndexFormat format, uint32_t offset)
        : buffer(buffer),
          format(format),
          offset(offset)
    {
    }

    void execute(GRICommandListBase & cmd_list);
};

GRICOMMAND_MACRO(GRICommandDrawIndexedPrimitive)
{
    uint32_t index_count;
    uint32_t first_index;
    int32_t  vertex_offset;
    GRICommandDrawIndexedPrimitive(uint32_t index_count, uint32_t first_index, int32_t vertex_offset)
        : index_count(index_count),
          first_index(first_index),
          vertex_offset(vertex_offset)
    {
    }

    void execute(GRICommandListBase & cmd_list);
};

GRICOMMAND_MACRO(GRICommandSetUniformBuffer)
{
    GRIBuffer*     buffer;
    uint32_t       slot;
    GRIShaderStage stage;
    uint32_t       offset;
    GRICommandSetUniformBuffer(GRIBuffer * buffer, uint32_t slot, GRIShaderStage stage, uint32_t offset)
        : buffer(buffer),
          slot(slot),
          stage(stage),
          offset(offset)
    {
    }

    void execute(GRICommandListBase & cmd_list);
};

GRICOMMAND_MACRO(GRICommandSetTexture)
{
    GRITexture2D*  texture;
    uint32_t       slot;
    GRIShaderStage stage;
    GRICommandSetTexture(GRITexture2D * texture, uint32_t slot, GRIShaderStage stage)
        : texture(texture),
          slot(slot),
          stage(stage)
    {
    }
    void execute(GRICommandListBase & cmd_list);
};

GRICOMMAND_MACRO(GRICommandSetGraphicsPipelineState)
{
    GRIPipelineState* pipeline_state;
    GRICommandSetGraphicsPipelineState(GRIPipelineState * pipeline_state)
        : pipeline_state(pipeline_state)
    {
    }

    void execute(GRICommandListBase & cmd_list);
};

GRICOMMAND_MACRO(GRICommandDrawPrimitive)
{
    uint32_t vertex_count;
    uint32_t first_vertex;
    GRICommandDrawPrimitive(uint32_t vertex_count, uint32_t first_vertex)
        : vertex_count(vertex_count),
          first_vertex(first_vertex)
    {
    }

    void execute(GRICommandListBase & cmd_list);
};

class GRICommandList : public GRICommandListBase
{
private:
public:
    GRICommandList()
        : GRICommandListBase()
    {
    }

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

    inline void begin_frame()
    {
        ALLOC_COMMAND(GRICommandBeginFrame)();
    }

    inline void end_frame()
    {
        ALLOC_COMMAND(GRICommandEndFrame)();
    }

    inline void begin_drawing_viewport(GRIViewport* viewport, GRITexture2D* render_target)
    {
        ALLOC_COMMAND(GRICommandBeginDrawingViewport)(viewport, render_target);
    }

    inline void begin_render_pass(const GRIRenderPassInfo& info = GRIRenderPassInfo{})
    {
        ALLOC_COMMAND(GRICommandBeginRenderPass)(info);
    }

    inline void end_render_pass()
    {
        ALLOC_COMMAND(GRICommandEndRenderPass)();
    }

    inline void set_vertex_buffer(GRIBuffer* buffer, uint32_t offset = 0, uint32_t buffer_index = 0)
    {
        ALLOC_COMMAND(GRICommandSetVertexBuffer)(buffer, offset, buffer_index);
    }

    inline void set_index_buffer(GRIBuffer* buffer, GRIIndexFormat format = GRIIndexFormat::Uint16, uint32_t offset = 0)
    {
        ALLOC_COMMAND(GRICommandSetIndexBuffer)(buffer, format, offset);
    }

    inline void set_uniform_buffer(GRIBuffer* buffer, uint32_t slot, GRIShaderStage stage, uint32_t offset = 0)
    {
        ALLOC_COMMAND(GRICommandSetUniformBuffer)(buffer, slot, stage, offset);
    }

    inline void set_texture(GRITexture2D* texture, uint32_t slot, GRIShaderStage stage)
    {
        ALLOC_COMMAND(GRICommandSetTexture)(texture, slot, stage);
    }

    inline void set_graphics_pipeline_state(GRIPipelineState* pipeline_state)
    {
        ALLOC_COMMAND(GRICommandSetGraphicsPipelineState)(pipeline_state);
    }

    inline void draw_primitives(uint32_t vertex_count, uint32_t first_vertex = 0)
    {
        ALLOC_COMMAND(GRICommandDrawPrimitive)(vertex_count, first_vertex);
    }

    inline void draw_indexed_primitives(uint32_t index_count, uint32_t first_index = 0, int32_t vertex_offset = 0)
    {
        ALLOC_COMMAND(GRICommandDrawIndexedPrimitive)(index_count, first_index, vertex_offset);
    }
};

class GRICommandListExecutor
{
private:
    GRICommandList m_command_list;

public:
    inline GRICommandList& get_command_list()
    {
        return m_command_list;
    }

    void submit();
};
} // namespace Ignis