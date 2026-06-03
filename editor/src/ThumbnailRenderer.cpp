#include "edpch.h"
#include "ThumbnailRenderer.h"
#include "EditorShaderCache.h"
#include <Ignis/Rendering/RenderSystem.h>
#include <Ignis/Rendering/Renderer.h>
#include <Ignis/Rendering/StaticGeometryBatcher.h>
#include <Ignis/Rendering/GRI/GRIDefinitions.h>

namespace Ignis
{

static constexpr uint32_t k_thumbnail_dim    = 80;
static constexpr uint32_t k_instance_slot    = 28;
static constexpr uint32_t k_uniform_buf_size = 8192;

struct ThumbFrameUniforms
{
    Math::Mat4f view_projection;
};

struct ThumbInstanceData
{
    Math::Mat4f world_matrix;
    uint32_t    material_index = 0;
    uint32_t    padding[3]     = {};
};

static Math::Mat4f make_thumbnail_mvp(Math::Vec3f center, float radius)
{
    using namespace Math;
    const Vec3f eye  = center + Vec3f{0.55f, 0.40f, 1.10f} * (radius * 2.5f);
    const Vec3f up   = Vec3f{0.0f, 1.0f, 0.0f};
    const Mat4f view = look_at(eye, center, up);
    const Mat4f proj = perspective(0.7854f, 1.0f, radius * 0.01f, radius * 20.0f);
    return proj * view;
}

SharedPtr<RenderMesh> ThumbnailRenderer::make_sphere_mesh()
{
    constexpr uint32_t k_lat = 12;
    constexpr uint32_t k_lon = 24;

    struct Vertex
    {
        float px, py, pz;
        float nx, ny, nz;
        float u, v;
    };

    Vector<Vertex>   verts;
    Vector<uint32_t> indices;
    verts.reserve((k_lat + 1) * (k_lon + 1));
    indices.reserve(k_lat * k_lon * 6);

    for (uint32_t i = 0; i <= k_lat; ++i)
    {
        const float phi = static_cast<float>(i) / k_lat * 3.14159265f;
        const float sp  = std::sin(phi);
        const float cp  = std::cos(phi);
        for (uint32_t j = 0; j <= k_lon; ++j)
        {
            const float theta = static_cast<float>(j) / k_lon * 6.28318530f;
            const float st    = std::sin(theta);
            const float ct    = std::cos(theta);
            Vertex      v;
            v.px = sp * ct;
            v.py = cp;
            v.pz = sp * st;
            v.nx = v.px;
            v.ny = v.py;
            v.nz = v.pz;
            v.u  = static_cast<float>(j) / k_lon;
            v.v  = static_cast<float>(i) / k_lat;
            verts.push_back(v);
        }
    }

    for (uint32_t i = 0; i < k_lat; ++i)
    {
        for (uint32_t j = 0; j < k_lon; ++j)
        {
            const uint32_t a = i * (k_lon + 1) + j;
            const uint32_t b = a + 1;
            const uint32_t c = a + (k_lon + 1);
            const uint32_t d = c + 1;
            indices.push_back(a);
            indices.push_back(c);
            indices.push_back(b);
            indices.push_back(b);
            indices.push_back(c);
            indices.push_back(d);
        }
    }

    return RenderMesh::create(verts.data(), static_cast<uint32_t>(verts.size() * sizeof(Vertex)), indices.data(),
                              static_cast<uint32_t>(indices.size()), GRIIndexFormat::Uint32, {0, 0, 0}, 1.0f);
}

void ThumbnailRenderer::init(EditorShaderCache& /*shaders*/)
{
    m_uniform_alloc.init(k_uniform_buf_size);

    m_sphere_mesh = make_sphere_mesh();

    GRITexture2DDesc depth_desc;
    depth_desc.width  = k_thumbnail_dim;
    depth_desc.height = k_thumbnail_dim;
    depth_desc.format = GRIPixelFormat::Depth32Float;
    m_shared_depth_rt = RenderSystem::get_gri()->create_texture2d(depth_desc);
}

void ThumbnailRenderer::shutdown()
{
    m_queue.clear();
    m_sphere_mesh.reset();
    m_shared_depth_rt.reset();
    m_uniform_alloc.shutdown();
}

bool ThumbnailRenderer::has_pending() const
{
    return !m_queue.empty();
}

void ThumbnailRenderer::enqueue(ThumbnailRenderRequest req)
{
    if (req.mesh && req.output_rt)
    {
        m_queue.push_back(req);
    }
}

void ThumbnailRenderer::flush()
{
    if (m_queue.empty() || !m_shared_depth_rt)
    {
        return;
    }

    GRICommandList& cmd = RenderSystem::get_command_list();

    m_uniform_alloc.begin_frame();

    RGTextureHandle depth_handle = m_builder.import_texture("thumb_depth", m_shared_depth_rt.get());

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_queue.size()); ++i)
    {
        const ThumbnailRenderRequest& req = m_queue[i];

        RGTextureHandle color_handle = m_builder.import_texture("thumb_color", req.output_rt);

        m_builder.write_render_target(0, color_handle, RGColorAttachmentDesc::clear({0.18f, 0.18f, 0.20f, 1.0f}));
        m_builder.write_depth_stencil(depth_handle, {GRILoadAction::Clear, GRIStoreAction::DontCare, 1.0f});

        const Math::Mat4f  mvp = make_thumbnail_mvp(req.bounds_center, req.bounds_radius);
        ThumbFrameUniforms fu;
        fu.view_projection                         = mvp;
        FrameUniformAllocator::Allocation fu_alloc = m_uniform_alloc.allocate(&fu, sizeof(fu));

        ThumbInstanceData inst;
        inst.world_matrix                            = Math::Mat4f::identity();
        inst.material_index                          = 0;
        FrameUniformAllocator::Allocation inst_alloc = m_uniform_alloc.allocate(&inst, sizeof(inst));

        GRIPipelineState* pso = req.material ? req.material->get_pipeline_state() : nullptr;

        GRIBuffer* vb = req.mesh->get_vertex_buffer() ? req.mesh->get_vertex_buffer()
                                                      : StaticGeometryBatcher::get().get_global_vb();
        GRIBuffer* ib =
            req.mesh->get_index_buffer() ? req.mesh->get_index_buffer() : StaticGeometryBatcher::get().get_global_ib();

        const MeshSlot       slot        = req.mesh->get_mesh_slot();
        const uint32_t       index_count = req.mesh->get_index_count() ? req.mesh->get_index_count() : slot.index_count;
        const int32_t        base_vertex = slot.base_vertex;
        const uint32_t       first_index = slot.first_index;
        const GRIIndexFormat idx_fmt     = req.mesh->get_index_format();

        m_builder.add_pass(
            "ThumbnailPass",
            [fu_alloc, inst_alloc, pso, vb, ib, idx_fmt, index_count, first_index, base_vertex](GRICommandList& c)
            {
                if (!pso || !vb || !ib)
                {
                    return;
                }
                c.set_graphics_pipeline_state(pso);
                c.set_uniform_buffer(fu_alloc.buffer, static_cast<uint32_t>(UniformSlot::FrameData),
                                     GRIShaderStage::Vertex, fu_alloc.offset);
                c.set_uniform_buffer(fu_alloc.buffer, static_cast<uint32_t>(UniformSlot::FrameData),
                                     GRIShaderStage::Pixel, fu_alloc.offset);
                c.set_vertex_buffer(inst_alloc.buffer, inst_alloc.offset, k_instance_slot);
                c.set_vertex_buffer(vb);
                c.set_index_buffer(ib, idx_fmt);
                c.draw_indexed_primitives_instanced(index_count, 1, 0, first_index, base_vertex);
            });
    }

    m_builder.execute(cmd);
    m_uniform_alloc.end_frame();
    m_queue.clear();
}

} // namespace Ignis
