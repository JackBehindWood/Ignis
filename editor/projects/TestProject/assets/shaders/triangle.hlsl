#pragma pack_matrix(column_major)

struct FrameUniforms
{
    float4x4 view_projection;
};

struct TransformUniforms
{
    float4x4 transform;
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

ConstantBuffer<FrameUniforms>     g_frame     : register(b0);
ConstantBuffer<TransformUniforms> g_transform : register(b1);
Texture2D    g_texture : register(t0);
SamplerState g_sampler : register(s0);

VertexOut VSMain(VertexIn input)
{
    VertexOut output;
    output.position = mul(g_frame.view_projection, mul(g_transform.transform, float4(input.position, 1.0)));
    output.uv       = input.uv;
    return output;
}

float4 PSMain(VertexOut input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv);
}
