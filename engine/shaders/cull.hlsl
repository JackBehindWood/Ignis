#pragma pack_matrix(column_major)

struct FrustumPlane
{
    float3 normal;
    float  d;
};

struct CullConstants
{
    FrustumPlane planes[6];      // 96 bytes
    uint         instance_count; //  4 bytes
    uint3        _pad;           // 12 bytes
};  // 112 bytes

struct GPUCullInstance
{
    float3 world_center;
    float  world_radius;
    uint   entity_index;
    uint3  _pad;
};

// Each binding in its own space — msl_buffer = desc_set (one resource per space, no collision).
ConstantBuffer<CullConstants>     g_cull    : register(b0, space4); // Metal buffer 4
StructuredBuffer<GPUCullInstance> g_input   : register(t0, space5); // Metal buffer 5
RWStructuredBuffer<uint>          g_visible : register(u0, space6); // Metal buffer 6
RWStructuredBuffer<uint>          g_counter : register(u1, space7); // Metal buffer 7

bool sphere_in_frustum(float3 center, float radius)
{
    for (int i = 0; i < 6; ++i)
    {
        if (dot(g_cull.planes[i].normal, center) + g_cull.planes[i].d < -radius)
        {
            return false;
        }
    }
    return true;
}

[numthreads(64, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    const uint idx = id.x;
    if (idx >= g_cull.instance_count) { return; }

    const GPUCullInstance inst = g_input[idx];
    if (!sphere_in_frustum(inst.world_center, inst.world_radius)) { return; }

    uint slot;
    InterlockedAdd(g_counter[0], 1u, slot);
    g_visible[slot] = inst.entity_index;
}
