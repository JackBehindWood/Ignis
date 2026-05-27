#include "igpch.h"
#include "Renderer.h"

#include "Ignis/Rendering/RenderSystem.h"

namespace Ignis
{
void Renderer::init(const RendererConfig& config)
{
    s_config = config;
    s_material_factory.set_render_target_format(config.render_target_format);
    s_material_factory.set_depth_format(config.depth_format);
    s_frame_alloc.init(config.uniform_buffer_size);
}

void Renderer::shutdown()
{
    s_frame_alloc.shutdown();
    s_material_factory.clear();
    s_resource_cache.clear();
}

void Renderer::begin_frame(GRIViewport* viewport)
{
    s_frame_alloc.begin_frame();

    GRICommandList& cmd = RenderSystem::get_command_list();
    cmd.begin_frame();
    cmd.begin_drawing_viewport(viewport, nullptr);
}

void Renderer::end_frame()
{
    GRICommandList& cmd = RenderSystem::get_command_list();
    cmd.end_frame();
    RenderSystem::submit();

    s_frame_alloc.end_frame();
}

void Renderer::bind_frame_data(GRICommandListBase& cmd_list, const void* data, uint32_t size)
{
    auto alloc = s_frame_alloc.allocate(data, size);
    GRICommandList::get(cmd_list).set_uniform_buffer(alloc.buffer, static_cast<uint32_t>(UniformSlot::FrameData),
                                                     GRIShaderStage::Vertex, alloc.offset);
}

void Renderer::bind_transform(GRICommandListBase& cmd_list, const void* data, uint32_t size)
{
    auto alloc = s_frame_alloc.allocate(data, size);
    GRICommandList::get(cmd_list).set_uniform_buffer(alloc.buffer, static_cast<uint32_t>(UniformSlot::Transform),
                                                     GRIShaderStage::Vertex, alloc.offset);
}

void Renderer::evict(uint64_t key)
{
    s_resource_cache.evict(key);
}

void Renderer::clear_pipeline_cache()
{
    s_material_factory.clear();
}
} // namespace Ignis
