#pragma pack_matrix(column_major)

Texture2D<float4> g_hdr     : register(t0);
SamplerState      g_sampler : register(s0);

struct VSOut
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD;
};

VSOut VSMain(uint vid : SV_VertexID)
{
    float2 uv = float2((vid << 1) & 2, vid & 2);
    VSOut  o;
    o.pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    o.uv  = uv;
    return o;
}

static float3 aces_filmic(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 PSMain(VSOut input) : SV_TARGET
{
    float3 hdr = g_hdr.Sample(g_sampler, input.uv).rgb;
    float3 ldr = aces_filmic(hdr);
    return float4(pow(ldr, 1.0 / 2.2), 1.0);
}
