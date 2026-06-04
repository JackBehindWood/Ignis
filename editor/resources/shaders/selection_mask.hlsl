#pragma pack_matrix(column_major)

struct FrameUniforms
{
    float4x4 view_projection;
};

struct GPUInstanceData
{
    float4x4 world_matrix;
    uint     material_index;
    uint3    padding;
};

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

ConstantBuffer<FrameUniforms>     g_frame     : register(b0);
StructuredBuffer<GPUInstanceData> g_instances : register(t0, space28);

VertexOut VSMain(VertexIn input, uint instanceID : SV_InstanceID)
{
    VertexOut o;
    o.position = mul(g_frame.view_projection, mul(g_instances[instanceID].world_matrix, float4(input.position, 1.0)));
    return o;
}

float4 PSMain() : SV_TARGET
{
    return float4(1.0, 0.0, 0.0, 1.0);
}
