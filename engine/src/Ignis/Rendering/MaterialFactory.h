#pragma once

#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/Shaders/RenderShader.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"

namespace Ignis
{
class MaterialFactory
{
public:
    // Returns a shared, params-free Material cached by (vs, ps, layout, rt_fmt, depth_fmt, depth_write, blend_mode).
    SharedPtr<Material> get_or_create(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps, const String& layout,
                                      GRIPixelFormat rt_fmt, GRIPixelFormat depth_fmt, bool depth_write = true,
                                      GRIBlendMode blend_mode = GRIBlendMode::None);

    // Reuses cached PSO, attaches caller-supplied params buffer. Result is not cached.
    SharedPtr<Material> create_with_params(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps, const String& layout,
                                           GRIPixelFormat rt_fmt, GRIPixelFormat depth_fmt, bool depth_write,
                                           GRIBlendMode blend_mode, GRIBufferPtr params);

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
        GRIShader*     vs;
        GRIShader*     ps; // nullptr for depth-only PSOs
        String         layout;
        GRIPixelFormat rt_format    = GRIPixelFormat::BGRA8Unorm;
        GRIPixelFormat depth_format = GRIPixelFormat::Depth32Float;
        bool           depth_write  = true;
        GRIBlendMode   blend_mode   = GRIBlendMode::None;

        bool operator==(const CacheKey& o) const
        {
            return vs == o.vs && ps == o.ps && layout == o.layout && rt_format == o.rt_format &&
                   depth_format == o.depth_format && depth_write == o.depth_write && blend_mode == o.blend_mode;
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
            h ^= std::hash<bool>{}(k.depth_write) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<uint32_t>{}(static_cast<uint32_t>(k.blend_mode)) + 0x9e3779b9 + (h << 6) + (h >> 2);
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
