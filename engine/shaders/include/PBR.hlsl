static const float PI = 3.14159265358979323846;

float D_GGX(float NdotH, float a2)
{
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denom * denom, 1e-4);
}

float G_SchlickGGX(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
    float k = roughness + 1.0;
    k       = (k * k) / 8.0;
    return G_SchlickGGX(NdotV, k) * G_SchlickGGX(NdotL, k);
}

float3 F_Schlick(float VdotH, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - VdotH), 5.0);
}

float3 F_SchlickRoughness(float NdotV, float3 F0, float roughness)
{
    float3 r = max((float3)(1.0 - roughness), F0);
    return F0 + (r - F0) * pow(saturate(1.0 - NdotV), 5.0);
}
