#include "edpch.h"
#include "EditorSceneOverlay.h"
#include "EditorResourceCache.h"

#include <Ignis/Scene/Components/Components.h>
#include <Ignis/Rendering/Renderer.h>
#include <Ignis/Rendering/RenderSystem.h>
#include <Ignis/Rendering/RenderMesh.h>
#include <Ignis/Rendering/RenderGraph/RGBuilder.h>
#include <Ignis/Rendering/Shaders/ShaderCache.h>
#include <Ignis/Rendering/StaticGeometryBatcher.h>
#include <Ignis/Rendering/GRI/GRICommandList.h>
#include <Ignis/Scene/RenderScene.h>

namespace Ignis
{

namespace
{

constexpr const char* k_sel_mask_hlsl = R"(
#include <Ignis.hlsl>
struct VertexIn { float3 position : POSITION; float3 normal : NORMAL; float2 uv : TEXCOORD; };
struct VertexOut { float4 position : SV_POSITION; };
VertexOut VSMain(VertexIn input, uint instanceID : SV_InstanceID)
{
    VertexOut o;
    o.position = mul(g_frame.view_projection, mul(g_instances[instanceID].world_matrix, float4(input.position, 1.0)));
    return o;
}
float4 PSMain(VertexOut input) : SV_TARGET { return float4(1.0, 0.0, 0.0, 1.0); }
)";

} // namespace

void EditorSceneOverlay::resize(uint32_t w, uint32_t h)
{
    if (w == 0 || h == 0 || (w == m_width && h == m_height))
    {
        return;
    }
    m_width  = w;
    m_height = h;

    GRITexture2DDesc desc;
    desc.width          = w;
    desc.height         = h;
    desc.num_mip_levels = 1;
    desc.format         = GRIPixelFormat::RGBA8Unorm;
    m_sel_mask_rt       = RenderSystem::get_gri()->create_texture2d(desc);
}

void EditorSceneOverlay::ensure_sel_material()
{
    if (m_sel_material)
    {
        return;
    }

    ShaderCompilerOptions opts;
    opts.stages[0] = {GRIShaderStage::Vertex};
    opts.stages[1] = {GRIShaderStage::Pixel};
    opts.count     = 2;

    SharedPtr<RenderShader> vs =
        ShaderCache::get().get_or_compile(String(k_sel_mask_hlsl), "_sel_mask", GRIShaderStage::Vertex, opts);
    SharedPtr<RenderShader> ps =
        ShaderCache::get().get_or_compile(String(k_sel_mask_hlsl), "_sel_mask", GRIShaderStage::Pixel, opts);
    if (!vs || !ps)
    {
        return;
    }

    GRIDepthStencilDesc ds;
    ds.depth_write = false;
    m_sel_material = Renderer::get_material_factory().get_or_create(vs, ps, "standard_mesh", GRIPixelFormat::RGBA8Unorm,
                                                                    Renderer::get_config().depth_format, ds, {}, {});
}

void EditorSceneOverlay::draw_grid(RGTextureHandle color, RGTextureHandle depth, RGBuilder& builder)
{
    Material* grid_mat = EditorResourceCache::get().get_material(EditorMaterial::Grid).get();
    if (!grid_mat)
    {
        return;
    }
    builder.write_render_target(0, color, RGColorAttachmentDesc::load());
    builder.read_depth_stencil(depth, {GRILoadAction::Load, GRIStoreAction::DontCare, 1.0f});
    builder.add_pass("EditorGrid", RGPassType::Graphics,
                     [grid_mat](GRICommandList& cmd)
                     {
                         Renderer::bind_frame_data(cmd);
                         cmd.set_graphics_pipeline_state(grid_mat->get_pipeline_state());
                         cmd.draw_primitives(6);
                     });
}

RGTextureHandle EditorSceneOverlay::draw_selection_mask(Entity selected, RGTextureHandle depth, RGBuilder& builder)
{
    if (!selected.is_valid() || !m_sel_mask_rt)
    {
        return {};
    }
    if (!selected.has_component<MeshRendererComponent>())
    {
        return {};
    }

    const MeshRendererComponent& mrc      = selected.get_component<MeshRendererComponent>();
    const uint64_t               mesh_key = static_cast<uint64_t>(mrc.mesh_id);
    SharedPtr<RenderMesh>        mesh     = Renderer::get_resource_cache().find_mesh(mesh_key);
    if (!mesh)
    {
        return {};
    }

    ensure_sel_material();
    if (!m_sel_material)
    {
        return {};
    }

    if (!m_sel_instance_buf)
    {
        GRIBufferDesc desc;
        desc.size          = static_cast<uint32_t>(sizeof(GPUInstanceData));
        desc.usage         = static_cast<GRIBufferUsage>(static_cast<uint32_t>(GRIBufferUsage::VertexBuffer) |
                                                         static_cast<uint32_t>(GRIBufferUsage::Dynamic));
        m_sel_instance_buf = RenderSystem::get_gri()->create_buffer(desc);
    }

    const TransformComponent& tc = selected.get_component<TransformComponent>();
    GPUInstanceData           inst;
    inst.world_matrix   = tc.to_mat4();
    inst.material_index = 0;
    inst.padding[0] = inst.padding[1] = inst.padding[2] = 0;
    RenderSystem::get_gri()->update_buffer(m_sel_instance_buf.get(), &inst, sizeof(GPUInstanceData));

    RGTextureHandle mask_rt = builder.import_texture("sel_mask", m_sel_mask_rt.get());
    builder.write_render_target(0, mask_rt, RGColorAttachmentDesc::clear({0.0f, 0.0f, 0.0f, 0.0f}));
    builder.read_depth_stencil(depth, {GRILoadAction::Load, GRIStoreAction::DontCare, 1.0f});

    const MeshSlot      slot     = mesh->get_mesh_slot();
    SharedPtr<Material> sel_mat  = m_sel_material;
    GRIBuffer*          inst_buf = m_sel_instance_buf.get();
    builder.add_pass("SelectionMask", RGPassType::Graphics,
                     [slot, sel_mat, inst_buf](GRICommandList& cmd)
                     {
                         GRIBuffer* global_vb = StaticGeometryBatcher::get().get_global_vb();
                         GRIBuffer* global_ib = StaticGeometryBatcher::get().get_global_ib();
                         if (!global_vb || !global_ib || !inst_buf)
                         {
                             return;
                         }
                         Renderer::bind_frame_data(cmd);
                         cmd.set_vertex_buffer(inst_buf, 0, static_cast<uint32_t>(DefaultBindings::InstanceData));
                         cmd.set_graphics_pipeline_state(sel_mat->get_pipeline_state());
                         cmd.set_vertex_buffer(global_vb);
                         cmd.set_index_buffer(global_ib, GRIIndexFormat::Uint32);
                         cmd.draw_indexed_primitives_instanced(slot.index_count, 1, 0, slot.first_index,
                                                               slot.base_vertex);
                     });

    return mask_rt;
}

void EditorSceneOverlay::draw_outline_composite(RGTextureHandle color, RGTextureHandle mask_rt, RGBuilder& builder)
{
    GRITexture2D* mask_tex    = m_sel_mask_rt.get();
    Material*     outline_mat = EditorResourceCache::get().get_material(EditorMaterial::SelectionOutline).get();
    GRIBuffer*    params_buf  = EditorResourceCache::get().get_outline_params();
    if (!mask_tex || !outline_mat || !params_buf)
    {
        return;
    }
    builder.write_render_target(0, color, RGColorAttachmentDesc::load());
    builder.read_texture(mask_rt);
    builder.add_pass("OutlineComposite", RGPassType::Graphics,
                     [outline_mat, params_buf, mask_tex](GRICommandList& cmd)
                     {
                         cmd.set_graphics_pipeline_state(outline_mat->get_pipeline_state());
                         cmd.set_texture(mask_tex, 0, GRIShaderStage::Pixel);
                         cmd.set_uniform_buffer(params_buf, static_cast<uint32_t>(DefaultBindings::MaterialArgs),
                                                GRIShaderStage::Pixel);
                         cmd.draw_primitives(3);
                     });
}

} // namespace Ignis
