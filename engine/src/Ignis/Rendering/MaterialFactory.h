#pragma once

#include "Ignis/Rendering/Material.h"
#include "Ignis/Rendering/Shaders/RenderShader.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"

namespace Ignis
{
class MaterialFactory
{
public:
    // Returns a shared, params-free Material. PSO is cached by (vs, ps, layout).
    SharedPtr<Material> get_or_create(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps, const String& layout);

    // Reuses cached PSO, attaches caller-supplied params buffer. Result is not cached.
    SharedPtr<Material> create_with_params(SharedPtr<RenderShader> vs, SharedPtr<RenderShader> ps, const String& layout,
                                           GRIBufferPtr params);

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
        GRIShader* vs;
        GRIShader* ps;
        String     layout;

        bool operator==(const CacheKey& o) const
        {
            return vs == o.vs && ps == o.ps && layout == o.layout;
        }
    };

    struct CacheKeyHash
    {
        size_t operator()(const CacheKey& k) const
        {
            size_t h = std::hash<void*>{}(k.vs);
            h ^= std::hash<void*>{}(k.ps) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<std::string>{}(k.layout) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    GRIPipelineStatePtr build_pso(GRIShader* vs, GRIShader* ps, const String& layout);

    GRIPixelFormat m_rt_format    = GRIPixelFormat::BGRA8Unorm;
    GRIPixelFormat m_depth_format = GRIPixelFormat::Depth32Float;

    UnorderedMap<CacheKey, GRIPipelineStatePtr, CacheKeyHash> m_pso_cache;
    UnorderedMap<CacheKey, SharedPtr<Material>, CacheKeyHash> m_mat_cache;
};
} // namespace Ignis
