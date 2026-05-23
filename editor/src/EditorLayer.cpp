#include "edpch.h"
#include "EditorLayer.h"
#include "EditorAssetManager.h"
#include "Ignis/Rendering/ShaderCache.h"
#include "Ignis/Rendering/Renderer.h"
#include "Ignis/Rendering/RenderSystem.h"

namespace Ignis
{

    EditorLayer::EditorLayer()
        : Layer("EditorLayer")
    {
    }

    void EditorLayer::attach()
    {
        IG_INFO("EditorLayer attached");

        Renderer::init();

        EditorAssetManager& assets = EditorAssetManager::get();
        assets.set_root(Filesystem::current_path() / "resources");
        ShaderCache::get().set_cache_root(Filesystem::current_path() / "resources" / "cache" / "shaders");

        const AssetID texture_id = assets.import_texture("test.png");
        m_texture = assets.load_texture(texture_id);
        IG_CORE_ASSERT(m_texture, "Failed to load test.png");

        m_material_id = assets.import_material("triangle.igmat");
        m_mesh_id     = assets.import_mesh("triangle.obj");

        SharedPtr<AssetMesh> mesh = assets.load_mesh(m_mesh_id);
        IG_CORE_ASSERT(mesh, "Failed to load triangle.obj");
        m_index_count = static_cast<uint32_t>(mesh->get_indices().size());

        m_transform.transform[0]  = 1.0f;
        m_transform.transform[5]  = 1.0f;
        m_transform.transform[10] = 1.0f;
        m_transform.transform[15] = 1.0f;

        m_forward_pass.colour_targets[0].load_action  = GRILoadAction::Clear;
        m_forward_pass.colour_targets[0].store_action = GRIStoreAction::Store;
        m_forward_pass.colour_targets[0].clear_value  = { 0.1f, 0.1f, 0.1f, 1.0f };
    }

    void EditorLayer::detach()
    {
        IG_INFO("EditorLayer detached");
        Renderer::shutdown();
        m_texture = nullptr;
    }

    void EditorLayer::update(Timestep ts)
    {
        GRIViewport* viewport = Application::get().get_window().get_viewport();
        Renderer::begin_frame(viewport);

        GRICommandList& cmd = RenderSystem::get_command_list();
        cmd.begin_render_pass(m_forward_pass);

        cmd.set_texture(m_texture->get_render_texture()->get_texture(), 0, GRIShaderStage::Pixel);
        Renderer::bind_material(cmd, m_material_id);
        Renderer::bind_mesh(cmd, m_mesh_id);
        Renderer::bind_transform(cmd, &m_transform, sizeof(m_transform));
        cmd.draw_indexed_primitives(m_index_count);

        cmd.end_render_pass();
        Renderer::end_frame();
    }

    void EditorLayer::event(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) -> bool
        {
            if (e.get_key_code() == Key::F5)
            {
                IG_INFO("Recompiling assets...");
                EditorAssetManager::get().reload_all();
                return true;
            }
            return false;
        });
    }

    bool EditorLayer::key_pressed(KeyPressedEvent& e)
    {
        return false;
    }
}
