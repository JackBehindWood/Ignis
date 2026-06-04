#pragma pack_matrix(column_major)

struct VertexOut
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

Texture2D g_mask : register(t0);

struct OutlineParams
{
    float4 color;
};
ConstantBuffer<OutlineParams> g_params : register(b2);

VertexOut VSMain(uint vertex_id : SV_VertexID)
{
    // Full-screen triangle covering clip space with 3 vertices.
    float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    VertexOut o;
    o.position = float4(uv * 2.0 - 1.0, 0.0, 1.0);
    o.uv       = uv;
    return o;
}

float4 PSMain(VertexOut input) : SV_TARGET
{
    // TODO: replace with stencil-based outline once GRI stencil support lands.
    int2 px = int2(input.position.xy);

    float tl = g_mask.Load(int3(px + int2(-1,  1), 0)).r;
    float  t = g_mask.Load(int3(px + int2( 0,  1), 0)).r;
    float tr = g_mask.Load(int3(px + int2( 1,  1), 0)).r;
    float  l = g_mask.Load(int3(px + int2(-1,  0), 0)).r;
    float  r = g_mask.Load(int3(px + int2( 1,  0), 0)).r;
    float bl = g_mask.Load(int3(px + int2(-1, -1), 0)).r;
    float  b = g_mask.Load(int3(px + int2( 0, -1), 0)).r;
    float br = g_mask.Load(int3(px + int2( 1, -1), 0)).r;

    float gx = -tl - 2.0 * l - bl + tr + 2.0 * r + br;
    float gy =  tl + 2.0 * t + tr - bl - 2.0 * b - br;
    float edge = saturate(sqrt(gx * gx + gy * gy));

    return float4(g_params.color.rgb, edge * g_params.color.a);
}
