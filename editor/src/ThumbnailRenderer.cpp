#include "edpch.h"
#include "ThumbnailRenderer.h"
#include "EditorShaderCache.h"
#include <Ignis/Rendering/RenderSystem.h>
#include <Ignis/Rendering/Renderer.h>
#include <Ignis/Rendering/GRI/GRIDefinitions.h>

namespace Ignis
{

static constexpr uint32_t k_uniform_buf_size = 65536;

static void make_thumbnail_frame_data(Math::Vec3f center, float radius, GPUFrameData& out)
{
    using namespace Math;
    const Vec3f eye  = center + Vec3f{0.55f, 0.40f, 1.10f} * (radius * 2.5f);
    const Vec3f up   = Vec3f{0.0f, 1.0f, 0.0f};
    const Mat4f view = look_at(eye, center, up);
    const Mat4f proj = perspective(0.7854f, 1.0f, radius * 0.01f, radius * 20.0f);

    out                  = {};
    out.view_projection  = proj * view;
    out.camera_world_pos = eye;

    out.num_directional_lights          = 1;
    out.directional_lights[0].direction = normalized(
        Vec3f{0.577f, -0.577f, -0.577f}); // Note: our thumbnails seems to nog get lighten up for any direction
    out.directional_lights[0].intensity = 3.0f;
    out.directional_lights[0].color     = {1.0f, 1.0f, 1.0f};
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
        const float phi = static_cast<float>(i) / k_lat * Math::pi;
        const float sp  = Math::sin(phi);
        const float cp  = Math::cos(phi);
        for (uint32_t j = 0; j <= k_lon; ++j)
        {
            const float theta = static_cast<float>(j) / k_lon * Math::tau;
            const float st    = Math::sin(theta);
            const float ct    = Math::cos(theta);
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

    struct GPUInstanceData
    {
        float    world_matrix[16];
        uint32_t material_index;
        uint32_t _pad[3];
    };
    const GPUInstanceData identity = {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}, 0, {0, 0, 0}};
    GRIBufferDesc         inst_desc;
    inst_desc.size          = sizeof(GPUInstanceData);
    inst_desc.usage         = GRIBufferUsage::UniformBuffer;
    m_identity_instance_buf = RenderSystem::get_gri()->create_buffer(inst_desc, &identity);

    const uint32_t zero_idx = 0;
    GRIBufferDesc  vis_desc;
    vis_desc.size      = sizeof(uint32_t);
    vis_desc.usage     = GRIBufferUsage::UniformBuffer;
    m_zero_visible_buf = RenderSystem::get_gri()->create_buffer(vis_desc, &zero_idx);
}

void ThumbnailRenderer::shutdown()
{
    m_queue.clear();
    m_sphere_mesh.reset();
    m_identity_instance_buf.reset();
    m_zero_visible_buf.reset();
    m_uniform_alloc.shutdown();
}

bool ThumbnailRenderer::has_pending() const
{
    return !m_queue.empty();
}

void ThumbnailRenderer::enqueue(ThumbnailRenderRequest req)
{
    if (req.mesh && req.material && req.output_rt)
    {
        m_queue.push_back(req);
    }
}

void ThumbnailRenderer::flush()
{
    if (m_queue.empty())
    {
        return;
    }

    GRICommandList& cmd = RenderSystem::get_command_list();
    cmd.begin_frame();
    m_uniform_alloc.begin_frame();

    for (const ThumbnailRenderRequest& req : m_queue)
    {
        GPUFrameData fu;
        make_thumbnail_frame_data(req.bounds_center, req.bounds_radius, fu);
        FrameUniformAllocator::Allocation fu_alloc = m_uniform_alloc.allocate(&fu, sizeof(fu));

        GRIPipelineState*    pso         = req.material ? req.material->get_pipeline_state() : nullptr;
        GRIBuffer*           vb          = req.mesh->get_vertex_buffer();
        GRIBuffer*           ib          = req.mesh->get_index_buffer();
        const uint32_t       index_count = req.mesh->get_index_count();
        const GRIIndexFormat idx_fmt     = req.mesh->get_index_format();

        IG_ASSERT(pso, "ThumbnailRenderer: missing pipeline state for material");
        IG_ASSERT(vb, "ThumbnailRenderer: missing vertex buffer for mesh");
        IG_ASSERT(ib, "ThumbnailRenderer: missing index buffer for mesh");

        GRIRenderPassInfo pass_info;
        pass_info.colour_targets[0].render_target = req.output_rt;
        pass_info.colour_targets[0].load_action   = GRILoadAction::Clear;
        pass_info.colour_targets[0].store_action  = GRIStoreAction::Store;
        pass_info.colour_targets[0].clear_value   = {0.18f, 0.18f, 0.20f, 1.0f};
        pass_info.num_explicit_colour_targets     = 1;

        cmd.begin_render_pass(pass_info);
        cmd.set_graphics_pipeline_state(pso);
        cmd.set_uniform_buffer(fu_alloc.buffer, static_cast<uint32_t>(DefaultBindings::FrameData),
                               GRIShaderStage::Vertex, fu_alloc.offset);
        cmd.set_uniform_buffer(fu_alloc.buffer, static_cast<uint32_t>(DefaultBindings::FrameData),
                               GRIShaderStage::Pixel, fu_alloc.offset);

        if (auto ibl = Renderer::get_global_cache().get_irradiance_cube())
        {
            cmd.set_texture(ibl->get_texture(), 0, GRIShaderStage::Pixel);
        }
        if (auto pre = Renderer::get_global_cache().get_prefilter_cube())
        {
            cmd.set_texture(pre->get_texture(), 1, GRIShaderStage::Pixel);
        }
        if (auto lut = Renderer::get_global_cache().get_brdf_lut())
        {
            cmd.set_texture(lut->get_texture(), 2, GRIShaderStage::Pixel);
        }

        if (req.material->get_params_buffer())
        {
            cmd.set_uniform_buffer(req.material->get_params_buffer(),
                                   static_cast<uint32_t>(DefaultBindings::MaterialArgs), GRIShaderStage::Pixel);
        }

        cmd.set_uniform_buffer(m_identity_instance_buf.get(), static_cast<uint32_t>(DefaultBindings::InstanceData),
                               GRIShaderStage::Vertex);
        cmd.set_uniform_buffer(m_zero_visible_buf.get(), static_cast<uint32_t>(DefaultBindings::VisibleIndices),
                               GRIShaderStage::Vertex);
        cmd.set_vertex_buffer(vb);
        cmd.set_index_buffer(ib, idx_fmt);
        cmd.draw_indexed_primitives(index_count, 0, 0);
        cmd.end_render_pass();
    }

    m_uniform_alloc.end_frame();
    m_queue.clear();
    cmd.end_frame();
    RenderSystem::submit();
}

} // namespace Ignis
