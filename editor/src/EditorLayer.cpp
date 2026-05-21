#include "edpch.h"
#include "EditorLayer.h"
#include "EditorAssetManager.h"
#include "Ignis/Rendering/ShaderCache.h"
#include "Ignis/Rendering/Renderer.h"

namespace Ignis
{

    struct SceneUniforms
    {
        float transform[16];
    };

    static SceneUniforms make_identity_uniforms()
    {
        SceneUniforms u{};
        u.transform[0]  = 1.0f;
        u.transform[5]  = 1.0f;
        u.transform[10] = 1.0f;
        u.transform[15] = 1.0f;
        return u;
    }

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

        const AssetID material_id = assets.import_material("triangle.igmat");
        m_material = assets.load_material(material_id);
        IG_CORE_ASSERT(m_material, "Failed to load triangle material");

        const AssetID mesh_id = assets.import_mesh("triangle.obj");
        SharedPtr<AssetMesh> mesh = assets.load_mesh(mesh_id);
        IG_CORE_ASSERT(mesh, "Failed to load triangle.obj");

        m_mesh = RenderMesh::create(
            mesh->get_vertices().data(),
            static_cast<uint32_t>(mesh->get_vertices().size()),
            mesh->get_indices().data(),
            static_cast<uint32_t>(mesh->get_indices().size()));

        SceneUniforms uniforms = make_identity_uniforms();
        GRIBufferDesc ub_desc;
        ub_desc.size  = sizeof(SceneUniforms);
        ub_desc.usage = GRIBufferUsage::UniformBuffer;
        m_uniform_buffer = RenderSystem::get_gri()->create_buffer(ub_desc, &uniforms);
    }

    void EditorLayer::detach()
    {
        IG_INFO("EditorLayer detached");
        m_uniform_buffer = nullptr;
        m_mesh           = nullptr;
        m_material       = nullptr;
    }

    void EditorLayer::update(Timestep ts)
    {
        GRIViewport* viewport = Application::get().get_window().get_viewport();
        Renderer::begin(viewport, { 0.1f, 0.1f, 0.1f, 1.0f });
        Renderer::submit(m_mesh.get(), m_material->get_material(), m_uniform_buffer.get());
        Renderer::end();
    }

    void EditorLayer::event(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) -> bool
        {
            if (e.get_key_code() == Key::F5)
            {
                IG_INFO("Recompiling shaders...");
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