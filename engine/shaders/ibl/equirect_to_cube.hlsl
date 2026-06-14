#include <Ignis.hlsl>
#include <IBL.hlsl>

struct EquirectParams
{
    uint face_index;
    uint face_size;
    uint _pad[2];
};

ConstantBuffer<EquirectParams> g_params : register(b0, space8);
Texture2D<float4>              g_equirect : register(t0, space9);
RWTexture2D<float4>            g_output   : register(u1, space9);

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_params.face_size || id.y >= g_params.face_size) { return; }

    float3 dir   = cube_texel_to_direction(id.xy, g_params.face_index, float(g_params.face_size));
    float  phi   = atan2(dir.z, dir.x);
    float  theta = asin(clamp(dir.y, -1.0, 1.0));
    float2 uv    = float2(phi / (2.0 * 3.14159265) + 0.5, 0.5 - theta / 3.14159265);

    g_output[id.xy] = g_equirect.SampleLevel(g_ibl_sampler, uv, 0.0);
}
