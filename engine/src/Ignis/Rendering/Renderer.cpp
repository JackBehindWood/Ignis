#include "igpch.h"
#include "Renderer.h"

#include "Ignis/Asset/AssetManager.h"
#include "Ignis/Asset/AssetMesh.h"
#include "Ignis/Rendering/RenderMesh.h"
#include "Ignis/Rendering/Material.h"
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
        s_material_cache.clear();
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

    void Renderer::bind_mesh(GRICommandListBase& cmd_list, AssetID mesh_id)
    {
        const uint64_t key = static_cast<uint64_t>(mesh_id);

        SharedPtr<RenderMesh> mesh = s_resource_cache.find_mesh(key);
        if (!mesh)
        {
            SharedPtr<AssetMesh> asset = AssetManager::get().load_as<AssetMesh>(mesh_id);
            IG_CORE_ASSERT(asset, "bind_mesh: AssetMesh not found");
            mesh = RenderMesh::create(
                asset->get_vertices().data(),
                static_cast<uint32_t>(asset->get_vertices().size()),
                asset->get_indices().data(),
                static_cast<uint32_t>(asset->get_indices().size()));
            s_resource_cache.register_mesh(key, mesh);
        }

        GRICommandList& cmd = GRICommandList::get(cmd_list);
        cmd.set_vertex_buffer(mesh->get_vertex_buffer());
        cmd.set_index_buffer(mesh->get_index_buffer(), mesh->get_index_format());
    }

    void Renderer::bind_material(GRICommandListBase& cmd_list, AssetID material_id)
    {
        const uint64_t key = static_cast<uint64_t>(material_id);

        SharedPtr<AssetMaterial>& cached = s_material_cache[key];
        if (!cached)
            cached = AssetManager::get().load_as<AssetMaterial>(material_id);

        IG_CORE_ASSERT(cached, "bind_material: AssetMaterial not found");
        Material* mat = cached->get_material();

        GRICommandList& cmd = GRICommandList::get(cmd_list);
        cmd.set_graphics_pipeline_state(mat->get_pipeline_state());

        if (GRIBuffer* params = mat->get_params_buffer())
            cmd.set_uniform_buffer(params,
                static_cast<uint32_t>(UniformSlot::MaterialArgs),
                GRIShaderStage::Pixel);
    }

    void Renderer::bind_transform(GRICommandListBase& cmd_list, const void* data, uint32_t size)
    {
        auto alloc = s_frame_alloc.allocate(data, size);
        GRICommandList::get(cmd_list).set_uniform_buffer(
            alloc.buffer,
            static_cast<uint32_t>(UniformSlot::Transform),
            GRIShaderStage::Vertex,
            alloc.offset);
    }
} // namespace Ignis
