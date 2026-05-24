#include "edpch.h"
#include "EditorLayer.h"
#include "EditorAssetManager.h"
#include "Ignis/Rendering/Shaders/ShaderCache.h"
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

        EditorAssetManager& assets = EditorAssetManager::get();
        assets.set_root(Filesystem::current_path() / "resources");
        ShaderCache::get().set_cache_root(Filesystem::current_path() / "resources" / "cache" / "shaders");

        const AssetID texture_id  = assets.import_texture("test.png");
        m_texture = assets.load_texture(texture_id);
        IG_CORE_ASSERT(m_texture, "Failed to load test.png");

        const AssetID material_id = assets.import_material("triangle.igmat");
        const AssetID mesh_id     = assets.import_mesh("triangle.obj");

        Entity e = m_active_scene.create_entity();
        e.add_component<TransformComponent>();
        MeshComponent mc;
        mc.mesh_id     = mesh_id;
        mc.material_id = material_id;
        e.add_component<MeshComponent>(mc);
    }

    void EditorLayer::detach()
    {
        IG_INFO("EditorLayer detached");
        m_texture = nullptr;
    }

    void EditorLayer::update(Timestep ts)
    {
        GRIViewport* viewport = Application::get().get_window().get_viewport();
        Renderer::begin_frame(viewport);

        GRICommandList& cmd = RenderSystem::get_command_list();

        RGTextureHandle bb = m_builder.import_backbuffer();
        m_scene_renderer.render_scene(
            m_active_scene, m_builder, bb,
            m_texture->get_render_texture()->get_texture());

        m_builder.execute(cmd);

        Renderer::end_frame();
    }

    void EditorLayer::event(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(IG_BIND_EVENT_FN(EditorLayer::key_pressed));
    }

    bool EditorLayer::key_pressed(KeyPressedEvent& e)
    {
        IG_INFO("Key pressed: {0} ({1} repeats)", e.get_key_code(), e.get_repeat_count());
        if (e.get_key_code() == Key::F5)
        {
            IG_INFO("Recompiling assets...");
            EditorAssetManager::get().reload_all();
            IG_INFO("Assets reloaded");
            return true;
        }
        return false;
    }
}
