#pragma once

#include "Ignis/Rendering/GRI/GRIDefinitions.h"
#include "Ignis/Math/Math.h"
#include "Ignis/Rendering/GRI/GRICommandList.h"
#include "Ignis/Rendering/RenderResourceCache.h"
#include "Ignis/Rendering/MaterialFactory.h"
#include "Ignis/Rendering/FrameUniformAllocator.h"
#include "Ignis/Rendering/GlobalEngineCache.h"

namespace Ignis
{
class GRIViewport;

static constexpr uint32_t k_max_directional_lights = 4;
static constexpr uint32_t k_max_point_lights       = 64;

struct GPUDirectionalLight
{
    Math::Vec3f direction;
    float       intensity = 1.0f;
    Math::Vec3f color     = {1.0f, 1.0f, 1.0f};
    float       _pad      = 0.0f;
};
static_assert(sizeof(GPUDirectionalLight) == 32);

struct GPUPointLight
{
    Math::Vec3f position;
    float       radius    = 10.0f;
    Math::Vec3f color     = {1.0f, 1.0f, 1.0f};
    float       intensity = 1.0f;
};
static_assert(sizeof(GPUPointLight) == 32);

struct GPUFrameData
{
    Math::Mat4f         view_projection;
    Math::Vec3f         camera_world_pos;
    float               _pad0 = 0.0f;
    GPUDirectionalLight directional_lights[k_max_directional_lights];
    GPUPointLight       point_lights[k_max_point_lights];
    uint32_t            num_directional_lights = 0;
    uint32_t            num_point_lights       = 0;
    uint32_t            debug_mode             = 0;
    uint32_t            _pad1                  = 0;
};

struct RendererConfig
{
    GRIPixelFormat render_target_format = GRIPixelFormat::BGRA8Unorm;
    GRIPixelFormat depth_format         = GRIPixelFormat::Depth32Float;
    uint32_t       max_frames_in_flight = 1;
    uint32_t       uniform_buffer_size  = 4 * 1024 * 1024;
    Path           engine_shaders;
};
class Renderer
{
public:
    static void init(const RendererConfig& config = {});
    static void shutdown();

    static void begin_frame(GRIViewport* viewport);
    static void end_frame();

    static void upload_frame_data(const GPUFrameData& data);
    static void bind_frame_data(GRICommandListBase& cmd_list);

    // Evict a cached GPU resource by opaque cache key (derived from AssetID by the caller).
    static void evict(uint64_t key);

    // Invalidate all PSO cache entries (call when any shader is reloaded).
    static void clear_pipeline_cache();

    static RenderResourceCache& get_resource_cache()
    {
        return s_resource_cache;
    }
    static MaterialFactory& get_material_factory()
    {
        return s_material_factory;
    }
    static GlobalEngineCache& get_global_cache()
    {
        return s_global_cache;
    }
    static const RendererConfig& get_config()
    {
        return s_config;
    }
    static GRITexture2D* get_depth_texture()
    {
        return s_depth_texture;
    }

private:
    static void warmup_system_shaders(const Path& engine_shaders_root);

    inline static RendererConfig                    s_config;
    inline static GRITexture2D*                     s_depth_texture = nullptr;
    inline static FrameUniformAllocator::Allocation s_frame_data_alloc{};
    inline static RenderResourceCache               s_resource_cache;
    inline static MaterialFactory                   s_material_factory;
    inline static FrameUniformAllocator             s_frame_alloc;
    inline static GlobalEngineCache                 s_global_cache;
};
} // namespace Ignis
