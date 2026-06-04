#pragma pack_matrix(column_major)

struct FrameUniforms
{
    float4x4 view_projection;
};

ConstantBuffer<FrameUniforms> g_frame : register(b0);

struct VertexIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 world_normal : NORMAL;
    float2 uv : TEXCOORD;
};

VertexOut VSMain(VertexIn input)
{
    VertexOut output;
    output.position     = mul(g_frame.view_projection, float4(input.position, 1.0));
    output.world_normal = normalize(input.normal);
    output.uv           = input.uv;
    return output;
}

struct PixelIn
{
    float3 world_normal : NORMAL;
    float2 uv           : TEXCOORD;
};

float4 PSMain(PixelIn input) : SV_TARGET
{
    static const float3 k_sun_dir   = float3(0.4767, 0.7946, -0.3773);
    static const float3 k_sun_color = float3(1.00, 0.95, 0.85);
    static const float3 k_ambient   = float3(0.15, 0.18, 0.25);
    float3 albedo  = float3(1.0, 1.0, 1.0);
    float3 n       = normalize(input.world_normal);
    float  n_dot_l = saturate(dot(n, k_sun_dir));
    float3 color   = albedo * (k_ambient + k_sun_color * n_dot_l);
    return float4(color, 1.0);
}
