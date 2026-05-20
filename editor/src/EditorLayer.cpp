#include "edpch.h"
#include "EditorLayer.h"
#include "EditorAssetManager.h"

namespace Ignis
{
    struct Vertex
    {
        float position[3];
        float color[4];
    };

    static const Vertex k_triangle_vertices[3] =
    {
        {{ 0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
    };

    static const uint16_t k_indices[3] = { 0, 1, 2 };

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

        const AssetID shader_id = assets.import_shader("triangle.hlsl");
        m_shader = assets.load_shader(shader_id);

        IG_CORE_ASSERT(m_shader, "Failed to compile/load triangle shader");

        GRI* gri = RenderSystem::get_gri();

        // Vertex buffer
        GRIBufferDesc vb_desc;
        vb_desc.size  = sizeof(k_triangle_vertices);
        vb_desc.usage = GRIBufferUsage::VertexBuffer;
        m_vertex_buffer = gri->create_buffer(vb_desc, k_triangle_vertices);

        // Index buffer
        GRIBufferDesc ib_desc;
        ib_desc.size  = sizeof(k_indices);
        ib_desc.usage = GRIBufferUsage::IndexBuffer;
        m_index_buffer = gri->create_buffer(ib_desc, k_indices);

        // Uniform buffer
        SceneUniforms uniforms = make_identity_uniforms();
        GRIBufferDesc ub_desc;
        ub_desc.size  = sizeof(SceneUniforms);
        ub_desc.usage = GRIBufferUsage::UniformBuffer;
        m_uniform_buffer = gri->create_buffer(ub_desc, &uniforms);

        // Vertex declaration: position (float3) + color (float4), interleaved in one buffer
        GRIVertexDeclaration vd;
        vd.elements[0] = { GRIVertexElementSemantic::Position, GRIVertexElementFormat::Float3, offsetof(Vertex, position), 0 };
        vd.elements[1] = { GRIVertexElementSemantic::Color,    GRIVertexElementFormat::Float4, offsetof(Vertex, color),    0 };
        vd.num_elements = 2;
        vd.bindings[0]  = { 0, sizeof(Vertex) };
        vd.num_bindings = 1;

        GRIPipelineStateDesc pso_desc;
        pso_desc.vertex_shader        = m_shader->get_render_shader().get_vertex_shader();
        pso_desc.pixel_shader         = m_shader->get_render_shader().get_pixel_shader();
        pso_desc.vertex_declaration   = &vd;
        pso_desc.render_target_format = GRIPixelFormat::RGBA8Unorm;
        pso_desc.depth_stencil_format = GRIPixelFormat::Depth32Float;
        pso_desc.primitive_topology   = GRIPrimitiveTopology::TriangleList;
        m_pipeline_state = gri->create_graphics_pipeline_state(pso_desc);
    }

    void EditorLayer::detach()
    {
        IG_INFO("EditorLayer detached");
        m_pipeline_state = nullptr;
        m_uniform_buffer = nullptr;
        m_index_buffer   = nullptr;
        m_vertex_buffer  = nullptr;
        m_shader         = nullptr;
    }

    void EditorLayer::update(Timestep ts)
    {
        GRIViewport* viewport    = Application::get().get_window().get_viewport();
        GRICommandList& cmd_list = RenderSystem::get_command_list();

        cmd_list.begin_frame();
        cmd_list.begin_drawing_viewport(viewport, nullptr);

        GRIRenderPassInfo rp;
        rp.colour_targets[0].load_action  = GRILoadAction::Clear;
        rp.colour_targets[0].store_action = GRIStoreAction::Store;
        rp.colour_targets[0].clear_value  = { 0.1f, 0.1f, 0.1f, 1.0f };
        cmd_list.begin_render_pass(rp);

        cmd_list.set_graphics_pipeline_state(m_pipeline_state.get());
        cmd_list.set_vertex_buffer(m_vertex_buffer.get());
        cmd_list.set_index_buffer(m_index_buffer.get());
        cmd_list.set_uniform_buffer(m_uniform_buffer.get(), 1, GRIShaderStage::Vertex);
        cmd_list.draw_indexed_primitives(3);

        cmd_list.end_render_pass();

        cmd_list.end_frame();
        RenderSystem::submit();
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
