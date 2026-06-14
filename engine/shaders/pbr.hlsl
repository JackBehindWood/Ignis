#include "Ignis.hlsl"
#include "PBR.hlsl"

struct VertexIn
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

struct VertexOut
{
    float4 sv_position  : SV_POSITION;
    float3 world_pos    : POSITION;
    float3 world_normal : NORMAL;
    float2 uv           : TEXCOORD;
};

VertexOut VSMain(VertexIn input, uint instance_id : SV_InstanceID)
{
    uint     entity_id = g_visible_indices[instance_id];
    float4x4 world     = g_instances[entity_id].world_matrix;
    float3x3 world_rot = float3x3(world[0].xyz, world[1].xyz, world[2].xyz);

    float4 world_pos = mul(world, float4(input.position, 1.0));

    VertexOut o;
    o.sv_position  = mul(g_frame.view_projection, world_pos);
    o.world_pos    = world_pos.xyz;
    o.world_normal = normalize(mul(world_rot, input.normal));
    o.uv           = input.uv;
    return o;
}

struct PixelIn
{
    float3 world_pos    : POSITION;
    float3 world_normal : NORMAL;
    float2 uv           : TEXCOORD;
};

float4 PSMain(PixelIn input) : SV_TARGET
{
    float4 albedo_sample    = g_material_textures[g_material.albedo_tex].Sample(
                                  g_material_samplers[g_material.albedo_tex], input.uv);
    float3 albedo           = albedo_sample.rgb;

    float3 normal_ts = g_material_textures[g_material.normal_tex].Sample(
                           g_material_samplers[g_material.normal_tex], input.uv).xyz * 2.0 - 1.0;

    float roughness  = g_material_textures[g_material.roughness_tex].Sample(
                           g_material_samplers[g_material.roughness_tex], input.uv).r;
    float metallic   = g_material_textures[g_material.metallic_tex].Sample(
                           g_material_samplers[g_material.metallic_tex], input.uv).r;
    float ao         = g_material_textures[g_material.ao_tex].Sample(
                           g_material_samplers[g_material.ao_tex], input.uv).r;
    float3 emissive  = g_material_textures[g_material.emissive_tex].Sample(
                           g_material_samplers[g_material.emissive_tex], input.uv).rgb;

    float3 N = normalize(input.world_normal);
    float3 V = normalize(g_frame.camera_world_pos - input.world_pos);

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    float NdotV = saturate(dot(N, V));
    float a2    = roughness * roughness * roughness * roughness;

    float3 Lo = float3(0.0, 0.0, 0.0);

    for (uint i = 0; i < g_frame.num_directional_lights; ++i)
    {
        float3 L       = normalize(-g_frame.directional_lights[i].direction);
        float3 H       = normalize(V + L);
        float  NdotL   = saturate(dot(N, L));
        float  NdotH   = saturate(dot(N, H));
        float  VdotH   = saturate(dot(V, H));
        float3 radiance = g_frame.directional_lights[i].color * g_frame.directional_lights[i].intensity;

        float3 F  = F_Schlick(VdotH, F0);
        float  D  = D_GGX(NdotH, a2);
        float  G  = G_Smith(NdotV, NdotL, roughness);
        float3 kD = (1.0 - F) * (1.0 - metallic);

        Lo += (kD * albedo / PI + D * F * G / max(4.0 * NdotV * NdotL, 1e-4)) * radiance * NdotL;
    }

    for (uint j = 0; j < g_frame.num_point_lights; ++j)
    {
        float3 to_light  = g_frame.point_lights[j].position - input.world_pos;
        float  dist      = length(to_light);
        float3 L         = to_light / max(dist, 1e-4);
        float3 H         = normalize(V + L);
        float  NdotL     = saturate(dot(N, L));
        float  NdotH     = saturate(dot(N, H));
        float  VdotH     = saturate(dot(V, H));
        float  radius    = max(g_frame.point_lights[j].radius, 1e-4);
        float  atten     = saturate(1.0 - (dist / radius) * (dist / radius));
        atten           *= atten;
        float3 radiance  = g_frame.point_lights[j].color * g_frame.point_lights[j].intensity * atten;

        float3 F  = F_Schlick(VdotH, F0);
        float  D  = D_GGX(NdotH, a2);
        float  G  = G_Smith(NdotV, NdotL, roughness);
        float3 kD = (1.0 - F) * (1.0 - metallic);

        Lo += (kD * albedo / PI + D * F * G / max(4.0 * NdotV * NdotL, 1e-4)) * radiance * NdotL;
    }

    // IBL ambient stub — replaced by split-sum IBL in Phase 3
    static const float k_ambient_intensity = 0.03;
    float3 ambient = albedo * k_ambient_intensity * ao;

    float3 color = ambient + Lo + emissive;
    return float4(color, albedo_sample.a);
}
