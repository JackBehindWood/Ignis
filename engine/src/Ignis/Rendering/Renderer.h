#pragma once

#include "Ignis/Rendering/GRI/GRIDefinitions.h"
#include "Ignis/Rendering/GRI/GRICommandList.h"
#include "Ignis/Rendering/RenderResourceCache.h"
#include "Ignis/Rendering/MaterialFactory.h"
#include "Ignis/Rendering/FrameUniformAllocator.h"

namespace Ignis
{
    class GRIViewport;

    enum class UniformSlot : uint32_t
    {
        FrameData    = 0,
        Transform    = 1,
        MaterialArgs = 2,
    };

    struct RendererConfig
    {
        GRIPixelFormat render_target_format = GRIPixelFormat::BGRA8Unorm;
        GRIPixelFormat depth_format         = GRIPixelFormat::Depth32Float;
        uint32_t       max_frames_in_flight = 1;
        uint32_t       uniform_buffer_size  = 4 * 1024 * 1024;
    };

    class Renderer
    {
    public:
        static void init(const RendererConfig& config = {});
        static void shutdown();

        static void begin_frame(GRIViewport* viewport);
        static void end_frame();

        static void bind_transform(GRICommandListBase& cmd_list, const void* data, uint32_t size);

        // Evict a cached GPU resource by opaque cache key (derived from AssetID by the caller).
        static void evict(uint64_t key);

        // Invalidate all PSO cache entries (call when any shader is reloaded).
        static void clear_pipeline_cache();

        static RenderResourceCache& get_resource_cache()   { return s_resource_cache; }
        static MaterialFactory&     get_material_factory() { return s_material_factory; }

    private:
        inline static RendererConfig        s_config;
        inline static RenderResourceCache   s_resource_cache;
        inline static MaterialFactory       s_material_factory;
        inline static FrameUniformAllocator s_frame_alloc;
    };
} // namespace Ignis
