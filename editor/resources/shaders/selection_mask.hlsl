#include <Ignis.hlsl>

struct VertexIn
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

struct VertexOut
{
    float4 position : SV_POSITION;
};

VertexOut VSMain(VertexIn input, uint instanceID : SV_InstanceID)
{
    uint entity_id = g_visible_indices[instanceID];
    VertexOut o;
    o.position = mul(g_frame.view_projection, mul(g_instances[entity_id].world_matrix, float4(input.position, 1.0)));
    return o;
}

float4 PSMain() : SV_TARGET
{
    return float4(1.0, 0.0, 0.0, 1.0);
}
