#include "edpch.h"
#include "EditorLayer.h"
#include "EditorAssetManager.h"
#include "Ignis/Asset/AssetManager.h"
#include "Ignis/Asset/AssetTexture2D.h"
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

        AssetManagerConfig v2_cfg;
        v2_cfg.compiled_root = assets.cache_dir();
        AssetManager::get().init(v2_cfg);

        AssetManager::get().set_reload_callback([](AssetID id) {
            Renderer::evict(static_cast<uint64_t>(id));
            if (AssetManager::get().get_metadata(id).Type == AssetType::Shader)
                Renderer::clear_pipeline_cache();
        });

        m_texture_id = assets.import_texture("test.png");
        AssetManager::get().load_deferred(m_texture_id);

        const AssetID material_id = assets.import_material("triangle.igmat");
        const AssetID mesh_id     = assets.import_mesh("triangle.obj");
        AssetManager::get().load_deferred(mesh_id);
        AssetManager::get().load_deferred(material_id);

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
        AssetManager::get().shutdown();
    }

    void EditorLayer::update(Timestep ts)
    {
        AssetManager::get().update(2.0f);

        GRIViewport* viewport = Application::get().get_window().get_viewport();
        Renderer::begin_frame(viewport);

        GRICommandList& cmd = RenderSystem::get_command_list();

        SharedPtr<AssetTexture2D> texture = AssetManager::get().get_asset_as<AssetTexture2D>(m_texture_id);

        if (!texture)
        {
            texture = static_pointer_cast<AssetTexture2D>(AssetManager::get().get_fallback(AssetType::Texture2D));
        }

        RGTextureHandle bb = m_builder.import_backbuffer();
        if (texture)
        {
            m_scene_renderer.render_scene(m_active_scene, m_builder, bb, texture->get_render_texture()->get_texture());
        }

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
            Renderer::get_resource_cache().clear();
            Renderer::clear_pipeline_cache();
            AssetManager::get().reload_all();
            IG_INFO("Assets reloaded");
            return true;
        }
        return false;
    }
}
