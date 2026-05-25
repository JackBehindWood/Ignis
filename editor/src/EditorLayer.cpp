#include "edpch.h"
#include "EditorLayer.h"
#include "EditorAssetManager.h"
#include "Ignis/Asset/AssetManager.h"
#include "Ignis/Asset/AssetTexture2D.h"
#include "Ignis/Rendering/RenderTexture2D.h"
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

    AssetManager::get().add_reload_callback(
        [](AssetID id)
        {
            Renderer::evict(static_cast<uint64_t>(id));
            if (AssetManager::get().get_metadata(id).Type == AssetType::Shader)
            {
                Renderer::clear_pipeline_cache();
            }
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

    SharedPtr<AssetTexture2D> tex_asset = AssetManager::get().get_asset_as<AssetTexture2D>(m_texture_id);
    if (!tex_asset)
    {
        tex_asset = static_pointer_cast<AssetTexture2D>(AssetManager::get().get_fallback(AssetType::Texture2D));
    }

    GRITexture2D* gri_texture = nullptr;
    if (tex_asset)
    {
        const uint64_t             tex_key   = static_cast<uint64_t>(tex_asset->get_id());
        SharedPtr<RenderTexture2D> cached_rt = Renderer::get_resource_cache().find_texture(tex_key);
        if (!cached_rt)
        {
            const auto&      pixels = tex_asset->get_pixels();
            GRITexture2DDesc desc;
            desc.width             = tex_asset->get_width();
            desc.height            = tex_asset->get_height();
            desc.num_mip_levels    = 1;
            desc.format            = static_cast<GRIPixelFormat>(tex_asset->get_format());
            desc.initial_data      = pixels.data();
            desc.initial_data_size = static_cast<uint32_t>(pixels.size());

            if (GRITexture2DPtr gri_tex = RenderSystem::get_gri()->create_texture2d(desc))
            {
                cached_rt = create_shared<RenderTexture2D>(std::move(gri_tex), desc.width, desc.height, desc.format);
                Renderer::get_resource_cache().register_texture(tex_key, cached_rt);
            }
        }
        if (cached_rt)
        {
            gri_texture = cached_rt->get_texture();
        }
    }

    RGTextureHandle bb = m_builder.import_backbuffer();
    if (gri_texture)
    {
        m_scene_renderer.render_scene(m_active_scene, m_builder, bb, gri_texture);
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
} // namespace Ignis
