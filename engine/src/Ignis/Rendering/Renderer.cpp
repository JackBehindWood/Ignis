#include "igpch.h"
#include "Renderer.h"

#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/VertexDeclarationRegistry.h"

namespace Ignis
{

static constexpr uint32_t k_default_vb_slot = 29;

static void register_builtin_vertex_layouts()
{
    if (VertexDeclarationRegistry::get().find("standard_mesh"))
    {
        return;
    }
    auto vd          = create_shared<GRIVertexDeclaration>();
    vd->num_elements = 3;
    vd->elements[0]  = {GRIVertexElementSemantic::Position, GRIVertexElementFormat::Float3, 0, k_default_vb_slot};
    vd->elements[1]  = {GRIVertexElementSemantic::Normal, GRIVertexElementFormat::Float3, 12, k_default_vb_slot};
    vd->elements[2]  = {GRIVertexElementSemantic::TexCoord, GRIVertexElementFormat::Float2, 24, k_default_vb_slot};
    vd->num_bindings = 1;
    vd->bindings[0]  = {k_default_vb_slot, 32};
    VertexDeclarationRegistry::get().register_layout("standard_mesh", std::move(vd));
}

void Renderer::init(const RendererConfig& config)
{
    s_config = config;
    s_material_factory.set_render_target_format(config.render_target_format);
    s_material_factory.set_depth_format(config.depth_format);
    s_frame_alloc.init(config.uniform_buffer_size);
    register_builtin_vertex_layouts();
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
    s_depth_texture = viewport ? RenderSystem::get_gri()->get_viewport_depth_texture(viewport) : nullptr;

    GRICommandList& cmd = RenderSystem::get_command_list();
    cmd.begin_frame();
    // cmd.begin_drawing_viewport(viewport, nullptr);
}

void Renderer::end_frame()
{
    GRICommandList& cmd = RenderSystem::get_command_list();
    cmd.end_frame();
    RenderSystem::submit();

    s_frame_alloc.end_frame();
}

void Renderer::upload_frame_data(const GPUFrameData& data)
{
    s_frame_data_alloc = s_frame_alloc.allocate(&data, sizeof(GPUFrameData));
}

void Renderer::bind_frame_data(GRICommandListBase& cmd_list)
{
    GRICommandList::get(cmd_list).set_uniform_buffer(s_frame_data_alloc.buffer,
                                                     static_cast<uint32_t>(UniformSlot::FrameData),
                                                     GRIShaderStage::Vertex, s_frame_data_alloc.offset);
    GRICommandList::get(cmd_list).set_uniform_buffer(s_frame_data_alloc.buffer,
                                                     static_cast<uint32_t>(UniformSlot::FrameData),
                                                     GRIShaderStage::Pixel, s_frame_data_alloc.offset);
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
