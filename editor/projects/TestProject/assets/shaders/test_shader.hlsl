#include <Ignis.hlsl>

struct VertexIn
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

struct VertexOut
{
    float4 position     : SV_POSITION;
    float3 world_normal : NORMAL;
    float2 uv           : TEXCOORD;
};

VertexOut VSMain(VertexIn input, uint instanceID : SV_InstanceID)
{
    uint     entity_id = g_visible_indices[instanceID];
    float4x4 world     = g_instances[entity_id].world_matrix;
    float3x3 world_rot = (float3x3)world;
    VertexOut output;
    output.position     = mul(g_frame.view_projection, mul(world, float4(input.position, 1.0)));
    output.world_normal = normalize(mul(world_rot, input.normal));
    output.uv           = input.uv;
    return output;
}

float4 PSMain(VertexOut input) : SV_TARGET
{
    static const float3 k_sun_dir    = float3(0.4767, 0.7946, -0.3773);
    static const float3 k_sun_color  = float3(1.00, 0.95, 0.85);
    static const float3 k_ambient    = float3(0.12, 0.15, 0.22);
    static const float3 k_base_color = float3(0.25, 0.55, 0.90);

    float3 n       = normalize(input.world_normal);
    float  n_dot_l = saturate(dot(n, k_sun_dir));
    float3 color   = k_base_color * (k_ambient + k_sun_color * n_dot_l);
    return float4(color, 1.0);
}
