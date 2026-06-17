#pragma once

#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/PBRMaterialParams.h"
#include "Ignis/Rendering/Shaders/RenderShader.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"

namespace Ignis
{
class MaterialFactory
{
public:
    // Returns a shared, params-free Material cached by (vs, ps, layout, rt_fmt, depth_fmt, depth_stencil, raster,
    // blend).
    SharedPtr<Material> get_or_create(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps, const String& layout,
                                      GRIPixelFormat rt_fmt, GRIPixelFormat depth_fmt,
                                      const GRIDepthStencilDesc& depth_stencil = {}, const GRIRasterDesc& raster = {},
                                      const GRIBlendDesc& blend = {});

    // Reuses cached PSO, attaches caller-supplied params buffer. Result is not cached.
    SharedPtr<Material> create_with_params(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps, const String& layout,
                                           GRIPixelFormat rt_fmt, GRIPixelFormat depth_fmt,
                                           const GRIDepthStencilDesc& depth_stencil, const GRIRasterDesc& raster,
                                           const GRIBlendDesc& blend, GRIBufferPtr params,
                                           PBRMaterialParams pbr_params = {});

    // Invalidates all PSO and Material entries referencing these shaders.
    void evict(RenderShader* vs, RenderShader* ps);

    void clear();

    void set_render_target_format(GRIPixelFormat fmt)
    {
        m_rt_format = fmt;
    }
    void set_depth_format(GRIPixelFormat fmt)
    {
        m_depth_format = fmt;
    }

private:
    struct CacheKey
    {
        GRIShader*          vs;
        GRIShader*          ps; // nullptr for depth-only PSOs
        String              layout;
        GRIPixelFormat      rt_format    = GRIPixelFormat::BGRA8Unorm;
        GRIPixelFormat      depth_format = GRIPixelFormat::Depth32Float;
        GRIDepthStencilDesc depth_stencil;
        GRIRasterDesc       raster;
        GRIBlendDesc        blend;

        bool operator==(const CacheKey& o) const
        {
            return vs == o.vs && ps == o.ps && layout == o.layout && rt_format == o.rt_format &&
                   depth_format == o.depth_format && depth_stencil.depth_test == o.depth_stencil.depth_test &&
                   depth_stencil.depth_write == o.depth_stencil.depth_write &&
                   depth_stencil.depth_func == o.depth_stencil.depth_func && raster.cull_mode == o.raster.cull_mode &&
                   raster.fill_mode == o.raster.fill_mode && raster.front_face_ccw == o.raster.front_face_ccw &&
                   blend.enable == o.blend.enable && blend.src_factor == o.blend.src_factor &&
                   blend.dst_factor == o.blend.dst_factor && blend.blend_op == o.blend.blend_op &&
                   blend.src_alpha == o.blend.src_alpha && blend.dst_alpha == o.blend.dst_alpha &&
                   blend.alpha_op == o.blend.alpha_op;
        }
    };

    struct CacheKeyHash
    {
        size_t operator()(const CacheKey& k) const
        {
            size_t h = std::hash<void*>{}(k.vs);
            h ^= std::hash<void*>{}(k.ps) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<std::string>{}(k.layout) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<uint32_t>{}(static_cast<uint32_t>(k.rt_format)) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<uint32_t>{}(static_cast<uint32_t>(k.depth_format)) + 0x9e3779b9 + (h << 6) + (h >> 2);
            const uint32_t ds = (uint32_t(k.depth_stencil.depth_test) << 0) |
                                (uint32_t(k.depth_stencil.depth_write) << 1) |
                                (uint32_t(k.depth_stencil.depth_func) << 2);
            h ^= std::hash<uint32_t>{}(ds) + 0x9e3779b9 + (h << 6) + (h >> 2);
            const uint32_t rs = (uint32_t(k.raster.cull_mode) << 0) | (uint32_t(k.raster.fill_mode) << 3) |
                                (uint32_t(k.raster.front_face_ccw) << 5);
            h ^= std::hash<uint32_t>{}(rs) + 0x9e3779b9 + (h << 6) + (h >> 2);
            const uint64_t bl = (uint64_t(k.blend.enable) << 0) | (uint64_t(k.blend.src_factor) << 1) |
                                (uint64_t(k.blend.dst_factor) << 5) | (uint64_t(k.blend.blend_op) << 9) |
                                (uint64_t(k.blend.src_alpha) << 13) | (uint64_t(k.blend.dst_alpha) << 17) |
                                (uint64_t(k.blend.alpha_op) << 21);
            h ^= std::hash<uint64_t>{}(bl) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    GRIPipelineStatePtr build_pso(const CacheKey& key);
    GRIPipelineState*   get_depth_pso(const CacheKey& forward_key);

    GRIPixelFormat m_rt_format    = GRIPixelFormat::BGRA8Unorm;
    GRIPixelFormat m_depth_format = GRIPixelFormat::Depth32Float;

    UnorderedMap<CacheKey, GRIPipelineStatePtr, CacheKeyHash> m_pso_cache;
    UnorderedMap<CacheKey, SharedPtr<Material>, CacheKeyHash> m_mat_cache;
};
} // namespace Ignis
