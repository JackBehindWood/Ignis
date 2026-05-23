#include "igpch.h"
#include "Renderer.h"
#include "Material.h"
#include "RenderMesh.h"
#include "RenderSystem.h"
#include "GRI/GRICommandList.h"

namespace Ignis
{
    void Renderer::begin(GRIViewport* viewport, GRIClearValue clear)
    {
        GRICommandList& cmd = RenderSystem::get_command_list();
        cmd.begin_frame();
        cmd.begin_drawing_viewport(viewport, nullptr);

        GRIRenderPassInfo rp;
        rp.colour_targets[0].load_action  = GRILoadAction::Clear;
        rp.colour_targets[0].store_action = GRIStoreAction::Store;
        rp.colour_targets[0].clear_value  = clear;
        cmd.begin_render_pass(rp);
    }

    void Renderer::submit(RenderMesh* mesh, Material* material, GRIBuffer* transform_ubo)
    {
        GRICommandList& cmd = RenderSystem::get_command_list();
        cmd.set_graphics_pipeline_state(material->get_pipeline_state());
        cmd.set_vertex_buffer(mesh->get_vertex_buffer());
        cmd.set_index_buffer(mesh->get_index_buffer(), mesh->get_index_format());
        if (transform_ubo)
            cmd.set_uniform_buffer(transform_ubo, 1, GRIShaderStage::Vertex);
        if (GRIBuffer* params = material->get_params_buffer())
            cmd.set_uniform_buffer(params, 2, GRIShaderStage::Pixel);
        cmd.draw_indexed_primitives(mesh->get_index_count());
    }

    void Renderer::end()
    {
        GRICommandList& cmd = RenderSystem::get_command_list();
        cmd.end_render_pass();
        cmd.end_frame();
        RenderSystem::submit();
    }
} // namespace Ignis
