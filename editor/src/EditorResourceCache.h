#pragma once

#include <Ignis.h>
#include <Ignis/Rendering/GRI/GRIDefinitions.h>
#include <Ignis/Rendering/GRI/GRIResource.h>
#include <Ignis/Rendering/Material.h>
#include "EditorShaderCache.h"
#include "EditorPrimitives.h"
#include "Project/IProjectObserver.h"
#include "ThumbnailRenderer.h"

namespace Ignis
{

enum class EditorMaterial : uint16_t
{
    Grid             = 0,
    SelectionOutline = 1,
    ThumbnailPreview = 2,
    Count
};

class EditorResourceCache : public IProjectObserver
{
public:
    struct ThumbnailResult
    {
        void* tex_id = nullptr; // ImTextureID — reinterpret_cast when passing to ImGui
        float uv0[2] = {0.0f, 0.0f};
        float uv1[2] = {1.0f, 1.0f};
        bool  ready  = false;
    };

    static EditorResourceCache& get()
    {
        static EditorResourceCache instance;
        return instance;
    }

    void init(const Path& engine_root);
    void shutdown();
    void reload();

    SharedPtr<Material> get_material(EditorMaterial mat) const;
    GRIBuffer*          get_outline_params() const;
    GRITexture2D*       get_folder_icon() const;
    GRITexture2D*       get_type_icon(const String& type_label) const;
    GRITexture2D*       get_play_icon() const;
    GRITexture2D*       get_pause_icon() const;
    GRITexture2D*       get_stop_icon() const;
    GRITexture2D*       get_gizmo_icon() const;

    ThumbnailResult request_thumbnail(const Path& path, const String& type_label);
    void            tick_thumbnails();
    bool            has_pending_render() const;
    void            flush_render_thumbnails();

    GRITexture2D* get_white_texture() const;

    static constexpr uint64_t k_fallback_mesh_key = 0xED17'0000'0000'0001ULL;
    AssetID                   get_fallback_mesh_id() const
    {
        return AssetID(k_fallback_mesh_key);
    }
    GRITexture2D* get_prim_thumbnail(uint64_t mesh_key) const;

    void on_project_opened(const ProjectContext& ctx) override;
    void on_project_closed() override;

private:
    EditorResourceCache() = default;
    void compile_all();
    void load_icons();

    class ThumbnailCache
    {
    public:
        ThumbnailResult request(const Path& path, const String& type_label);
        void            tick(ThumbnailRenderer& renderer, EditorShaderCache& shaders);
        void            promote_pending_render();
        void            clear();
        bool            has_pending_render() const;

    private:
        enum class State
        {
            Unloaded,
            Pending,
            PendingRender,
            Ready,
            Failed
        };
        struct Entry
        {
            AssetID                    asset_id;
            SharedPtr<AssetTexture2D>  tex_ref;
            SharedPtr<RenderTexture2D> render_tex_ref;
            GRITexture2D*              gri_tex = nullptr;
            GRITexture2DPtr            owned_rt;
            State                      state  = State::Unloaded;
            float                      uv0[2] = {0.0f, 0.0f};
            float                      uv1[2] = {1.0f, 1.0f};
            String                     type_label;
        };
        UnorderedMap<Path, Entry> m_entries;
    };

    ThumbnailCache    m_thumbnail_cache;
    ThumbnailRenderer m_thumb_renderer;

    EditorShaderCache                       m_shader_cache;
    mutable SharedMutex                     m_mutex;
    static constexpr size_t                 k_mat_count = static_cast<size_t>(EditorMaterial::Count);
    Array<SharedPtr<Material>, k_mat_count> m_materials{};
    GRIBufferPtr                            m_outline_params;
    GRITexture2DPtr                         m_folder_icon;
    GRITexture2DPtr                         m_tex_placeholder_icon;
    GRITexture2DPtr                         m_mesh_icon;
    GRITexture2DPtr                         m_shader_icon;
    GRITexture2DPtr                         m_scene_icon;
    GRITexture2DPtr                         m_white_texture;
    GRITexture2DPtr                         m_play_icon;
    GRITexture2DPtr                         m_pause_icon;
    GRITexture2DPtr                         m_stop_icon;
    GRITexture2DPtr                         m_gizmo_icon;
    SharedPtr<RenderMesh>                   m_fallback_mesh;
    Array<GRITexture2DPtr, static_cast<size_t>(EditorPrimitives::prim_count())> m_prim_thumbnails{};
    Path                                                                        m_engine_root;
};

} // namespace Ignis
