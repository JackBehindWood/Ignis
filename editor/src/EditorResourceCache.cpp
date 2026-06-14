#include "edpch.h"
#include "EditorResourceCache.h"
#include "EditorPrimitives.h"
#include "Asset/EditorAssetManager.h"
#include <Ignis/Asset/AssetTexture2D.h>
#include <Ignis/Asset/AssetMesh.h>
#include <Ignis/Asset/AssetMaterial.h>
#include <Ignis/Rendering/Renderer.h>
#include <Ignis/Rendering/RenderSystem.h>
#include <Ignis/Rendering/MaterialFactory.h>
#include <Ignis/Rendering/RenderResourceCache.h>
#include <Ignis/Rendering/RenderTexture2D.h>
#include <stb/stb_image.h>

namespace Ignis
{

GRITexture2D* EditorResourceCache::get_prim_thumbnail(uint64_t key) const
{
    constexpr uint64_t base  = EditorPrimitives::prim_mesh_key(0);
    constexpr uint64_t count = static_cast<uint64_t>(EditorPrimitives::prim_count());
    if (key < base || key >= base + count)
    {
        return nullptr;
    }
    SharedLock lock(m_mutex);
    return m_prim_thumbnails[static_cast<size_t>(key - base)].get();
}

EditorResourceCache::ThumbnailResult EditorResourceCache::ThumbnailCache::request(const Path&   path,
                                                                                  const String& type_label)
{
    auto it = m_entries.find(path);
    if (it == m_entries.end())
    {
        Entry e;
        e.type_label = type_label;

        AssetType at = AssetType::None;
        if (type_label == "TEX")
        {
            at = AssetType::Texture2D;
        }
        else if (type_label == "MSH")
        {
            at = AssetType::Mesh;
        }
        else if (type_label == "MAT")
        {
            at = AssetType::Material;
        }

        if (at != AssetType::None)
        {
            e.asset_id = EditorAssetManager::get().import_asset_at(path, at);
            EditorAssetManager::get().load_deferred(e.asset_id);
            e.state = State::Pending;
        }
        else
        {
            e.state = State::Failed;
        }
        it = m_entries.emplace(path, std::move(e)).first;
    }

    const Entry& e = it->second;
    if (e.state == State::Ready && e.gri_tex)
    {
        ThumbnailResult r;
        r.tex_id = e.gri_tex->get_native_handle();
        r.uv0[0] = e.uv0[0];
        r.uv0[1] = e.uv0[1];
        r.uv1[0] = e.uv1[0];
        r.uv1[1] = e.uv1[1];
        r.ready  = true;
        return r;
    }
    return {};
}

// TODO: get it rid of if else nesting!
void EditorResourceCache::ThumbnailCache::tick(ThumbnailRenderer& renderer, EditorShaderCache& shaders)
{
    constexpr uint32_t k_thumbnail_dim = 80;

    for (auto& [path, e] : m_entries)
    {
        if (e.state != State::Pending)
        {
            continue;
        }

        if (e.type_label == "TEX")
        {
            SharedPtr<AssetTexture2D> tex = EditorAssetManager::get().try_get_texture(e.asset_id);
            if (!tex)
            {
                continue;
            }
            const uint64_t             cache_key = static_cast<uint64_t>(e.asset_id);
            SharedPtr<RenderTexture2D> cached    = Renderer::get_resource_cache().find_texture(cache_key);
            if (!cached)
            {
                GRITexture2DDesc desc;
                desc.width             = tex->get_width();
                desc.height            = tex->get_height();
                desc.num_mip_levels    = 1;
                desc.format            = static_cast<GRIPixelFormat>(tex->get_format());
                desc.initial_data      = tex->get_pixels().data();
                desc.initial_data_size = static_cast<uint32_t>(tex->get_pixels().size());
                if (GRITexture2DPtr gri = RenderSystem::get_gri()->create_texture2d(desc))
                {
                    cached = create_shared<RenderTexture2D>(std::move(gri), desc.width, desc.height, desc.format);
                    Renderer::get_resource_cache().register_texture(cache_key, cached);
                }
            }
            if (!cached)
            {
                e.state = State::Failed;
                continue;
            }
            e.tex_ref         = tex;
            e.render_tex_ref  = cached;
            GRITexture2D* ptr = cached->get_texture();
            e.gri_tex         = ptr;
            const uint32_t w  = ptr->get_width();
            const uint32_t h  = ptr->get_height();
            if (w > h)
            {
                const float cx = static_cast<float>(w - h) / static_cast<float>(2 * w);
                e.uv0[0]       = cx;
                e.uv1[0]       = 1.0f - cx;
                e.uv0[1]       = 0.0f;
                e.uv1[1]       = 1.0f;
            }
            else if (h > w)
            {
                const float cy = static_cast<float>(h - w) / static_cast<float>(2 * h);
                e.uv0[0]       = 0.0f;
                e.uv1[0]       = 1.0f;
                e.uv0[1]       = cy;
                e.uv1[1]       = 1.0f - cy;
            }
            e.state = State::Ready;
        }
        else if (e.type_label == "MSH")
        {
            const uint64_t        key           = static_cast<uint64_t>(e.asset_id);
            SharedPtr<RenderMesh> render_mesh   = Renderer::get_resource_cache().find_mesh(key);
            Math::Vec3f           bounds_center = {};
            float                 bounds_radius = 1.0f;

            if (!render_mesh)
            {
                SharedPtr<AssetMesh> asset_mesh = EditorAssetManager::get().try_get_mesh(e.asset_id);
                if (!asset_mesh)
                {
                    continue;
                }
                bounds_center = asset_mesh->get_bounds_center();
                bounds_radius = asset_mesh->get_bounds_radius();
                render_mesh   = RenderMesh::create(
                    asset_mesh->get_vertices().data(), static_cast<uint32_t>(asset_mesh->get_vertices().size()),
                    asset_mesh->get_indices().data(), static_cast<uint32_t>(asset_mesh->get_indices().size()),
                    GRIIndexFormat::Uint32, bounds_center, bounds_radius);
                Renderer::get_resource_cache().register_mesh(key, render_mesh);
            }

            GRITexture2DDesc rt_desc;
            rt_desc.width  = k_thumbnail_dim;
            rt_desc.height = k_thumbnail_dim;
            rt_desc.format = GRIPixelFormat::RGBA8Unorm;
            e.owned_rt     = RenderSystem::get_gri()->create_texture2d(rt_desc);
            if (!e.owned_rt)
            {
                e.state = State::Failed;
                continue;
            }
            e.gri_tex = e.owned_rt.get();

            const SharedPtr<Material>& thumb_mat =
                EditorResourceCache::get().get_material(EditorMaterial::ThumbnailPreview);

            ThumbnailRenderRequest req;
            req.mesh          = render_mesh.get();
            req.material      = thumb_mat.get();
            req.output_rt     = e.owned_rt.get();
            req.bounds_center = bounds_center;
            req.bounds_radius = bounds_radius;
            renderer.enqueue(req);
            e.state = State::PendingRender;
        }
        else if (e.type_label == "MAT")
        {
            SharedPtr<AssetMaterial> asset_mat = EditorAssetManager::get().try_get_material(e.asset_id);
            if (!asset_mat)
            {
                continue;
            }

            GRITexture2DDesc rt_desc;
            rt_desc.width  = k_thumbnail_dim;
            rt_desc.height = k_thumbnail_dim;
            rt_desc.format = GRIPixelFormat::RGBA8Unorm;
            e.owned_rt     = RenderSystem::get_gri()->create_texture2d(rt_desc);
            if (!e.owned_rt)
            {
                e.state = State::Failed;
                continue;
            }
            e.gri_tex = e.owned_rt.get();

            SharedPtr<RenderShader> mat_vs =
                shaders.get_or_compile_path(asset_mat->get_shader_source(), GRIShaderStage::Vertex);
            SharedPtr<RenderShader> mat_ps =
                shaders.get_or_compile_path(asset_mat->get_shader_source(), GRIShaderStage::Pixel);

            SharedPtr<Material> compiled_mat;
            if (mat_vs && mat_ps)
            {
                GRIDepthStencilDesc ds;
                ds.depth_test  = false;
                ds.depth_write = false;
                GRIRasterDesc raster;
                raster.cull_mode = asset_mat->is_two_sided() ? GRICullMode::None : GRICullMode::Back;
                GRIBlendDesc blend;
                if (asset_mat->is_transparent())
                {
                    blend.enable     = true;
                    blend.src_factor = GRIBlendFactor::SrcAlpha;
                    blend.dst_factor = GRIBlendFactor::InvSrcAlpha;
                    blend.blend_op   = GRIBlendOp::Add;
                    blend.src_alpha  = GRIBlendFactor::One;
                    blend.dst_alpha  = GRIBlendFactor::InvSrcAlpha;
                    blend.alpha_op   = GRIBlendOp::Add;
                }
                compiled_mat = Renderer::get_material_factory().get_or_create(
                    mat_vs, mat_ps, asset_mat->get_vertex_layout(), GRIPixelFormat::RGBA8Unorm, GRIPixelFormat::Unknown,
                    ds, raster, blend);
            }

            ThumbnailRenderRequest req;
            req.mesh          = renderer.get_sphere_mesh();
            req.material      = compiled_mat.get();
            req.output_rt     = e.owned_rt.get();
            req.bounds_center = {};
            req.bounds_radius = 1.0f;
            renderer.enqueue(req);
            e.state = State::PendingRender;
        }
        else
        {
            e.state = State::Failed;
        }
    }
}

void EditorResourceCache::ThumbnailCache::promote_pending_render()
{
    for (auto& [path, e] : m_entries)
    {
        if (e.state == State::PendingRender)
        {
            IG_ASSERT(e.owned_rt, "Only owned render targets should be in pending render state");
            e.state = State::Ready;
        }
    }
}

void EditorResourceCache::ThumbnailCache::clear()
{
    m_entries.clear();
}

bool EditorResourceCache::ThumbnailCache::has_pending_render() const
{
    for (const auto& [path, e] : m_entries)
    {
        if (e.state == State::PendingRender)
        {
            return true;
        }
    }
    return false;
}

// ---- EditorResourceCache ---------------------------------------------------

void EditorResourceCache::init(const Path& engine_root)
{
    UniqueLock lock(m_mutex);
    m_engine_root = engine_root;
    m_shader_cache.init(engine_root / "shaders", engine_root / "cache" / "shaders");
    compile_all();
    load_icons();
    m_thumb_renderer.init(m_shader_cache);
}

void EditorResourceCache::shutdown()
{
    UniqueLock lock(m_mutex);
    m_thumb_renderer.shutdown();
    m_thumbnail_cache.clear();
    m_folder_icon.reset();
    m_tex_placeholder_icon.reset();
    m_mesh_icon.reset();
    m_shader_icon.reset();
    m_scene_icon.reset();
    m_white_texture.reset();
    m_play_icon.reset();
    m_pause_icon.reset();
    m_stop_icon.reset();
    m_gizmo_icon.reset();
    for (auto& t : m_prim_thumbnails)
    {
        t.reset();
    }
    if (m_fallback_mesh)
    {
        Renderer::get_resource_cache().evict(k_fallback_mesh_key);
        m_fallback_mesh.reset();
    }
    m_outline_params.reset();
    for (auto& mat : m_materials)
    {
        mat.reset();
    }
}

void EditorResourceCache::reload()
{
    UniqueLock lock(m_mutex);
    m_outline_params.reset();
    for (auto& mat : m_materials)
    {
        mat.reset();
    }
    compile_all();
}

SharedPtr<Material> EditorResourceCache::get_material(EditorMaterial mat) const
{
    SharedLock lock(m_mutex);
    return m_materials[static_cast<size_t>(mat)];
}

GRIBuffer* EditorResourceCache::get_outline_params() const
{
    SharedLock lock(m_mutex);
    return m_outline_params.get();
}

GRITexture2D* EditorResourceCache::get_folder_icon() const
{
    SharedLock lock(m_mutex);
    return m_folder_icon.get();
}

GRITexture2D* EditorResourceCache::get_type_icon(const String& type_label) const
{
    if (type_label == "TEX")
    {
        return m_tex_placeholder_icon.get();
    }
    if (type_label == "MSH")
    {
        return m_mesh_icon.get();
    }
    if (type_label == "SHD")
    {
        return m_shader_icon.get();
    }
    if (type_label == "SCN")
    {
        return m_scene_icon.get();
    }
    if (type_label == "MAT")
    {
        return m_mesh_icon.get();
    }
    return nullptr;
}

EditorResourceCache::ThumbnailResult EditorResourceCache::request_thumbnail(const Path& path, const String& type_label)
{
    return m_thumbnail_cache.request(path, type_label);
}

void EditorResourceCache::tick_thumbnails()
{
    m_thumbnail_cache.tick(m_thumb_renderer, m_shader_cache);
}

bool EditorResourceCache::has_pending_render() const
{
    return m_thumb_renderer.has_pending();
}

void EditorResourceCache::flush_render_thumbnails()
{
    m_thumb_renderer.flush();
    m_thumbnail_cache.promote_pending_render();
}

void EditorResourceCache::on_project_opened(const ProjectContext& ctx)
{
    m_shader_cache.on_project_opened(ctx);
}

void EditorResourceCache::on_project_closed()
{
    m_thumbnail_cache.clear();
    m_shader_cache.on_project_closed();
}

static GRITexture2DPtr make_solid_icon(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    const uint8_t    pixel[4] = {r, g, b, a};
    GRITexture2DDesc desc;
    desc.width             = 1;
    desc.height            = 1;
    desc.format            = GRIPixelFormat::RGBA8Unorm;
    desc.initial_data      = pixel;
    desc.initial_data_size = 4;
    return RenderSystem::get_gri()->create_texture2d(desc);
}

void EditorResourceCache::compile_all()
{
    const RendererConfig& cfg = Renderer::get_config();

    SharedPtr<RenderShader> grid_vs = m_shader_cache.get_or_compile("grid.hlsl", GRIShaderStage::Vertex);
    SharedPtr<RenderShader> grid_ps = m_shader_cache.get_or_compile("grid.hlsl", GRIShaderStage::Pixel);

    GRIBlendDesc grid_blend;
    grid_blend.enable     = true;
    grid_blend.src_factor = GRIBlendFactor::SrcAlpha;
    grid_blend.dst_factor = GRIBlendFactor::InvSrcAlpha;
    grid_blend.blend_op   = GRIBlendOp::Add;
    grid_blend.src_alpha  = GRIBlendFactor::One;
    grid_blend.dst_alpha  = GRIBlendFactor::InvSrcAlpha;
    grid_blend.alpha_op   = GRIBlendOp::Add;
    GRIRasterDesc grid_raster;
    grid_raster.cull_mode                                  = GRICullMode::None;
    m_materials[static_cast<size_t>(EditorMaterial::Grid)] = Renderer::get_material_factory().get_or_create(
        grid_vs, grid_ps, "", GRIPixelFormat::RGBA16Float, cfg.depth_format, {}, grid_raster, grid_blend);

    SharedPtr<RenderShader> outline_vs =
        m_shader_cache.get_or_compile("outline_composite.hlsl", GRIShaderStage::Vertex);
    SharedPtr<RenderShader> outline_ps = m_shader_cache.get_or_compile("outline_composite.hlsl", GRIShaderStage::Pixel);

    GRIDepthStencilDesc outline_ds;
    outline_ds.depth_test  = false;
    outline_ds.depth_write = false;
    GRIBlendDesc outline_blend;
    outline_blend.enable     = true;
    outline_blend.src_factor = GRIBlendFactor::SrcAlpha;
    outline_blend.dst_factor = GRIBlendFactor::InvSrcAlpha;
    outline_blend.blend_op   = GRIBlendOp::Add;
    outline_blend.src_alpha  = GRIBlendFactor::One;
    outline_blend.dst_alpha  = GRIBlendFactor::InvSrcAlpha;
    outline_blend.alpha_op   = GRIBlendOp::Add;
    GRIRasterDesc outline_raster;
    outline_raster.cull_mode                                           = GRICullMode::None;
    m_materials[static_cast<size_t>(EditorMaterial::SelectionOutline)] = Renderer::get_material_factory().get_or_create(
        outline_vs, outline_ps, "", GRIPixelFormat::RGBA16Float, GRIPixelFormat::Unknown, outline_ds, outline_raster,
        outline_blend);

    SharedPtr<RenderShader> thumb_vs = m_shader_cache.get_or_compile("thumbnail.hlsl", GRIShaderStage::Vertex);
    SharedPtr<RenderShader> thumb_ps = m_shader_cache.get_or_compile("thumbnail.hlsl", GRIShaderStage::Pixel);
    if (thumb_vs && thumb_ps)
    {
        GRIDepthStencilDesc thumb_ds;
        thumb_ds.depth_test  = false;
        thumb_ds.depth_write = false;
        GRIRasterDesc thumb_raster;
        thumb_raster.cull_mode = GRICullMode::Back;
        m_materials[static_cast<size_t>(EditorMaterial::ThumbnailPreview)] =
            Renderer::get_material_factory().get_or_create(thumb_vs, thumb_ps, "standard_mesh",
                                                           GRIPixelFormat::RGBA8Unorm, GRIPixelFormat::Unknown,
                                                           thumb_ds, thumb_raster, {});
    }

    if (!m_fallback_mesh)
    {
        m_fallback_mesh = ThumbnailRenderer::make_sphere_mesh();
        Renderer::get_resource_cache().register_mesh(k_fallback_mesh_key, m_fallback_mesh);
    }

    if (!m_outline_params)
    {
        struct OutlineParams
        {
            float color[4];
        };
        const OutlineParams outline_params = {0.26f, 0.59f, 1.0f, 1.0f};
        GRIBufferDesc       desc;
        desc.size        = sizeof(OutlineParams);
        desc.usage       = GRIBufferUsage::UniformBuffer;
        m_outline_params = RenderSystem::get_gri()->create_buffer(desc, &outline_params);
    }

    EditorPrimitives::init();

    const SharedPtr<Material>& thumb_mat = m_materials[static_cast<size_t>(EditorMaterial::ThumbnailPreview)];
    if (thumb_mat)
    {
        constexpr uint32_t k_dim = 80;
        for (int i = 0; i < EditorPrimitives::prim_count(); ++i)
        {
            if (!m_prim_thumbnails[i])
            {
                GRITexture2DDesc rt_desc;
                rt_desc.width        = k_dim;
                rt_desc.height       = k_dim;
                rt_desc.format       = GRIPixelFormat::RGBA8Unorm;
                m_prim_thumbnails[i] = RenderSystem::get_gri()->create_texture2d(rt_desc);
            }
            const uint64_t        key  = EditorPrimitives::prim_mesh_key(i);
            SharedPtr<RenderMesh> mesh = Renderer::get_resource_cache().find_mesh(key);
            if (m_prim_thumbnails[i] && mesh)
            {
                ThumbnailRenderRequest req;
                req.mesh          = mesh.get();
                req.material      = thumb_mat.get();
                req.output_rt     = m_prim_thumbnails[i].get();
                req.bounds_center = {};
                req.bounds_radius = 0.5f;
                m_thumb_renderer.enqueue(req);
            }
        }
    }
}

void EditorResourceCache::load_icons()
{
    auto load_png = [&](const Path& path) -> GRITexture2DPtr
    {
        int      w = 0, h = 0, channels = 0;
        stbi_uc* data = stbi_load(path.string().c_str(), &w, &h, &channels, STBI_rgb_alpha);
        if (!data)
        {
            IG_CORE_WARN("EditorResourceCache: failed to load {}", path.string());
            return nullptr;
        }
        GRITexture2DDesc desc;
        desc.width             = static_cast<uint32_t>(w);
        desc.height            = static_cast<uint32_t>(h);
        desc.format            = GRIPixelFormat::RGBA8Unorm;
        desc.initial_data      = data;
        desc.initial_data_size = static_cast<size_t>(w * h * 4);
        GRITexture2DPtr tex    = RenderSystem::get_gri()->create_texture2d(desc);
        stbi_image_free(data);
        return tex;
    };

    const Path icons = m_engine_root / "icons";
    m_folder_icon    = load_png(icons / "folder.png");
    m_play_icon      = load_png(icons / "play_icon.png");
    m_pause_icon     = load_png(icons / "pause_icon.png");
    m_stop_icon      = load_png(icons / "stop_icon.png");
    m_gizmo_icon     = load_png(icons / "gizmo_icon.png");

    m_tex_placeholder_icon = make_solid_icon(128, 128, 128, 255);
    m_mesh_icon            = make_solid_icon(0, 140, 140, 255);
    m_shader_icon          = make_solid_icon(220, 100, 0, 255);
    m_scene_icon           = make_solid_icon(100, 50, 200, 255);
    m_white_texture        = make_solid_icon(255, 255, 255, 255);
}

GRITexture2D* EditorResourceCache::get_play_icon() const
{
    SharedLock lock(m_mutex);
    return m_play_icon.get();
}

GRITexture2D* EditorResourceCache::get_pause_icon() const
{
    SharedLock lock(m_mutex);
    return m_pause_icon.get();
}

GRITexture2D* EditorResourceCache::get_stop_icon() const
{
    SharedLock lock(m_mutex);
    return m_stop_icon.get();
}

GRITexture2D* EditorResourceCache::get_gizmo_icon() const
{
    SharedLock lock(m_mutex);
    return m_gizmo_icon.get();
}

GRITexture2D* EditorResourceCache::get_white_texture() const
{
    SharedLock lock(m_mutex);
    return m_white_texture.get();
}

} // namespace Ignis
