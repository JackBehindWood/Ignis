#pragma once

#include "Ignis/Math/Math.h"
#include "Ignis/Math/Frustum.h"
#include "Ignis/Rendering/RenderMesh.h"
#include "Ignis/Rendering/Renderer.h"

namespace Ignis
{

inline constexpr uint32_t k_max_instances         = 4096;
inline constexpr uint32_t k_max_batches           = 512;
inline constexpr float    k_depth_range           = 1000.0f;
inline constexpr uint64_t k_fallback_material_key = 0xFFFF'FFFF'FFFF'FFFEull;
inline constexpr uint64_t k_white_texture_key     = 0xFFFF'FFFF'FFFF'FFFCull;

struct DrawIndexedArguments
{
    uint32_t index_count;
    uint32_t instance_count;
    uint32_t first_index;
    int32_t  base_vertex;
    uint32_t base_instance;
};

struct CullProxy
{
    Math::Vec3f world_center;
    float       world_radius;
    uint32_t    entity_index;
};

struct GPUInstanceData
{
    Math::Mat4f world_matrix;
    uint32_t    material_index;
    uint32_t    padding[3];
};

struct FrustumPlane
{
    Math::Vec3f normal;
    float       d;
};

struct GPUCullInstance
{
    Math::Vec3f world_center; // 12 bytes
    float       world_radius; //  4 bytes
    uint32_t    entity_index; //  4 bytes
    uint32_t    _pad[3];      // 12 bytes
}; // 32 bytes

struct GPUCullConstants
{
    FrustumPlane planes[6];      //  96 bytes
    uint32_t     instance_count; //   4 bytes
    uint32_t     _pad[3];        //  12 bytes
}; // 112 bytes

// Per-instance drawing proxy: no GRI handles. SceneRenderer resolves mat_key/texture_key to
// GRI resources via RenderResourceCache and constructs DrawBatch internally.
struct DrawProxy
{
    MeshSlot slot;         // batcher geometry location
    uint32_t entity_index; // index into RenderScene::instance_data[]
    uint16_t buffer_id;    // mesh cache ID (sort key + batch grouping)
    uint16_t material_id;  // material cache ID (sort key + batch grouping)
    uint16_t depth_pso_id; // material_id ^ 0x8000
    bool     is_transparent;
    uint64_t sort_key_depth; // pre-computed front-to-back key
    uint64_t sort_key_fwd;   // pre-computed state-minimised / back-to-front key
    uint64_t mat_key;        // material asset key → Material* via RenderResourceCache
    uint64_t texture_key;    // texture asset key; 0 = white fallback
};

struct RenderScene
{
    Vector<GPUInstanceData> instance_data; // indexed by DrawProxy::entity_index
    Vector<CullProxy>       cull_proxies;  // parallel to instance_data
    Vector<DrawProxy>       depth_proxies; // sorted front-to-back, opaques only
    Vector<DrawProxy>       fwd_proxies;   // sorted by fwd_key (opaque state-min, then transparents)
    GPUFrameData            frame_data;
    Math::Frustum           frustum; // camera frustum for GPU cull pass
};

} // namespace Ignis
