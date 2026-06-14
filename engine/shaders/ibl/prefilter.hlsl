#include <Ignis.hlsl>
#include <PBR.hlsl>
#include <IBL.hlsl>

struct PrefilterParams
{
    uint  face_index;
    uint  mip_size;
    float roughness;
    uint  num_samples;
    uint  env_res;
    uint  _pad[3];
};

ConstantBuffer<PrefilterParams> g_params   : register(b0, space8);
TextureCube<float4>             g_env_cube : register(t0, space9);
RWTexture2D<float4>             g_output   : register(u1, space9);

[numthreads(8, 8, 1)]
void CSPrefilter(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_params.mip_size || id.y >= g_params.mip_size) { return; }

    float3 N           = cube_texel_to_direction(id.xy, g_params.face_index, float(g_params.mip_size));
    float3 V           = N;
    float  roughness   = g_params.roughness;
    float  total_weight = 0.0;
    float3 prefiltered  = 0.0;

    for (uint i = 0; i < g_params.num_samples; ++i)
    {
        float2 Xi    = Hammersley(i, g_params.num_samples);
        float3 H     = ImportanceSampleGGX(Xi, N, roughness);
        float3 L     = normalize(2.0 * dot(V, H) * H - V);
        float  NdotL = saturate(dot(N, L));
        if (NdotL <= 0.0) { continue; }

        float NdotH = saturate(dot(N, H));
        float VdotH = saturate(dot(V, H));
        float a2    = roughness * roughness * roughness * roughness;

        float pdf    = D_GGX(NdotH, a2) * NdotH / (4.0 * VdotH + 1e-4);
        float omegaS = 1.0 / (float(g_params.num_samples) * pdf + 1e-4);
        float omegaP = 4.0 * PI / (6.0 * float(g_params.env_res) * float(g_params.env_res));
        float mip    = roughness == 0.0 ? 0.0 : 0.5 * log2(omegaS / omegaP) + 1.0;

        prefiltered  += g_env_cube.SampleLevel(g_ibl_sampler, L, mip).rgb * NdotL;
        total_weight += NdotL;
    }

    g_output[id.xy] = float4(prefiltered / max(total_weight, 1e-4), 1.0);
}
