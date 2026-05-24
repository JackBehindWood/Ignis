#include "igpch.h"
#include "SceneRenderer.h"
#include "Components.h"
#include "Ignis/Asset/AssetManager.h"
#include "Ignis/Asset/AssetMesh.h"
#include "Ignis/Asset/AssetMaterial.h"
#include "Ignis/Rendering/Renderer.h"
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/RenderGraph/RGBuilder.h"

namespace Ignis
{
    namespace
    {
        struct ScenePassParams
        {
            GRITexture2D* scene_texture;
        };
    }

    namespace Utils
    {
        static void emit (GRICommandList& cmd, const FrameDrawItem& item)
        {
            cmd.set_graphics_pipeline_state(item.material->get_pipeline_state());
            if (item.material->get_params_buffer())
                cmd.set_uniform_buffer(item.material->get_params_buffer(),
                                        static_cast<uint32_t>(UniformSlot::MaterialArgs),
                                        GRIShaderStage::Pixel);
            cmd.set_vertex_buffer(item.mesh->get_vertex_buffer());
            cmd.set_index_buffer(item.mesh->get_index_buffer(), item.mesh->get_index_format());
            Renderer::bind_transform(cmd, item.world.data(), sizeof(Math::Mat4f));
            cmd.draw_indexed_primitives(item.mesh->get_index_count());
        };
    }

    void SceneRenderer::render_scene(Scene& scene, RGBuilder& builder, RGTextureHandle backbuffer, GRITexture2D* scene_texture)
    {
        m_opaque.clear();
        m_transparent.clear();

        // --- Camera ---
        Math::Mat4f cam_view = Math::Mat4f::identity();
        Math::Mat4f cam_proj = Math::Mat4f::identity();
        {
            auto cv = scene.registry().view<CameraComponent>();
            if (!cv.empty())
            {
                const auto& cam = scene.registry().get<CameraComponent>(*cv.begin());
                cam_view = cam.view;
                cam_proj = cam.projection;
            }
        }
        const Math::Frustum frustum = Math::extract_frustum(cam_proj * cam_view);

        // --- Build draw lists ---
        // Cache miss path (first-use): upload mesh to GPU and register in RenderResourceCache.
        // Asset lifetimes are guaranteed stable for the frame duration by the AssetManager registry;
        // raw observing pointers are safe here.
        auto view = scene.registry().view<TransformComponent, MeshComponent>();
        for (auto [entity, transform, mesh_comp] : view.each())
        {
            if (!mesh_comp.is_visible)
                continue;

            const Math::Mat4f world = transform.transform.to_mat4();

            // --- Mesh resolve ---
            const RenderMesh* render_mesh = nullptr;
            {
                SharedPtr<RenderMesh> cached =
                    Renderer::get_resource_cache().find_mesh(uint64_t(mesh_comp.mesh_id));
                if (!cached)
                {
                    auto asset_mesh = AssetManager::get().load_as<AssetMesh>(mesh_comp.mesh_id);
                    if (!asset_mesh)
                        continue;

                    // Compute AABB-derived bounding sphere; assumes position at vertex offset 0.
                    Math::Vec3f bounds_center{};
                    float bounds_radius = 1.0e30f;
                    const uint32_t stride = asset_mesh->get_vertex_stride();
                    const Vector<uint8_t>& verts = asset_mesh->get_vertices();
                    if (stride >= sizeof(Math::Vec3f) && !verts.empty())
                    {
                        const uint32_t count = static_cast<uint32_t>(verts.size()) / stride;
                        Math::Vec3f mn{ 1.0e30f,  1.0e30f,  1.0e30f};
                        Math::Vec3f mx{-1.0e30f, -1.0e30f, -1.0e30f};
                        for (uint32_t i = 0; i < count; ++i)
                        {
                            const Math::Vec3f& p =
                                *reinterpret_cast<const Math::Vec3f*>(verts.data() + i * stride);
                            mn.x = Math::min(mn.x, p.x); mn.y = Math::min(mn.y, p.y); mn.z = Math::min(mn.z, p.z);
                            mx.x = Math::max(mx.x, p.x); mx.y = Math::max(mx.y, p.y); mx.z = Math::max(mx.z, p.z);
                        }
                        bounds_center = (mn + mx) * 0.5f;
                        bounds_radius = (mx - mn).length() * 0.5f;
                    }

                    cached = RenderMesh::create(
                        verts.data(),
                        static_cast<uint32_t>(verts.size()),
                        asset_mesh->get_indices().data(),
                        static_cast<uint32_t>(asset_mesh->get_indices().size()),
                        GRIIndexFormat::Uint32,
                        bounds_center, bounds_radius
                    );
                    Renderer::get_resource_cache().register_mesh(uint64_t(mesh_comp.mesh_id), cached);
                }
                render_mesh = cached.get(); // raw observer; cache holds ownership
            }

            // --- Frustum cull ---
            {
                const Math::Vec3f world_center =
                    (world * Math::Vec4f(render_mesh->get_bounds_center(), 1.0f)).xyz();
                const Math::Vec3f& s = transform.transform.scale;
                const float world_radius =
                    render_mesh->get_bounds_radius() * Math::max(s.x, Math::max(s.y, s.z));
                if (!Math::frustum_contains_sphere(frustum, world_center, world_radius))
                    continue;
            }

            // --- Material resolve ---
            const Material* mat = nullptr;
            {
                auto asset_mat = AssetManager::get().load_as<AssetMaterial>(mesh_comp.material_id);
                if (!asset_mat)
                    continue;
                mat = asset_mat->get_material(); // raw observer; AssetManager holds ownership
            }

            // --- Depth (view-space Z from world translation) ---
            const Math::Vec3f pos = transform.transform.position;
            const Math::Vec4f cam_row2 = cam_view.row(2);
            const float depth = cam_row2.x * pos.x
                               + cam_row2.y * pos.y
                               + cam_row2.z * pos.z
                               + cam_row2.w;

            FrameDrawItem item;
            item.world    = world;
            item.mesh     = render_mesh;
            item.material = mat;
            item.depth    = depth;

            if (mat->is_transparent())
                m_transparent.push_back(item);
            else
                m_opaque.push_back(item);
        }

        // --- Sort ---
        std::sort(m_opaque.begin(), m_opaque.end(),
            [](const FrameDrawItem& a, const FrameDrawItem& b) { return a.material < b.material; });
        std::sort(m_transparent.begin(), m_transparent.end(),
            [](const FrameDrawItem& a, const FrameDrawItem& b) { return a.depth > b.depth; });

        // --- Register pass ---
        ScenePassParams* params = builder.alloc_params<ScenePassParams>();
        params->scene_texture = scene_texture;

        builder.write_render_target(0, backbuffer, RGColorAttachmentDesc::clear({ 0.1f, 0.1f, 0.1f, 1.0f }));
        builder.add_pass("ForwardScene", params,
            [this](ScenePassParams* p, GRICommandList& cmd)
            {
                cmd.set_texture(p->scene_texture, 0, GRIShaderStage::Pixel);

                for (const FrameDrawItem& item : m_opaque)
                    Utils::emit(cmd, item);
                for (const FrameDrawItem& item : m_transparent)
                    Utils::emit(cmd, item);
            });
    }

} // namespace Ignis
