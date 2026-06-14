#include <Ignis.hlsl>
#include <PBR.hlsl>
#include <IBL.hlsl>

RWTexture2D<float2> g_output : register(u0, space8);

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    float NdotV     = (float(id.x) + 0.5) / 512.0;
    float roughness = (float(id.y) + 0.5) / 512.0;
    float3 V        = float3(sqrt(max(0.0, 1.0 - NdotV * NdotV)), 0.0, NdotV);

    float scale = 0.0;
    float bias  = 0.0;

    for (uint i = 0; i < 1024u; ++i)
    {
        float2 Xi   = Hammersley(i, 1024u);
        float3 H    = ImportanceSampleGGX(Xi, float3(0, 0, 1), roughness);
        float3 L    = normalize(2.0 * dot(V, H) * H - V);
        float NdotL = saturate(L.z);
        float NdotH = saturate(H.z);
        float VdotH = saturate(dot(V, H));
        if (NdotL <= 0.0) { continue; }

        float G     = G_Smith(NdotV, NdotL, roughness);
        float G_vis = G * VdotH / max(NdotH * NdotV, 1e-4);
        float Fc    = pow(1.0 - VdotH, 5.0);
        scale += (1.0 - Fc) * G_vis;
        bias  += Fc         * G_vis;
    }

    g_output[id.xy] = float2(scale, bias) / 1024.0;
}
