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
    float2 uv       : TEXCOORD;
};

ConstantBuffer<FrameUniforms>             g_frame     : register(b0);
StructuredBuffer<GPUInstanceData>         g_instances : register(t0, space28);
Texture2D    g_texture : register(t0);
SamplerState g_sampler : register(s0);

VertexOut VSMain(VertexIn input, uint instanceID : SV_InstanceID)
{
    float4x4 world = g_instances[instanceID].world_matrix;
    VertexOut output;
    output.position = mul(g_frame.view_projection, mul(world, float4(input.position, 1.0)));
    output.uv       = input.uv;
    return output;
}

float4 PSMain(VertexOut input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv);
}
