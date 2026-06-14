#include <Ignis.hlsl>
#include <IBL.hlsl>

struct IrradianceParams
{
    uint face_index;
    uint face_size;
    uint _pad[2];
};

ConstantBuffer<IrradianceParams> g_params   : register(b0, space8);
TextureCube<float4>              g_env_cube : register(t0, space9);
RWTexture2D<float4>              g_output   : register(u1, space9);

static const float PI_LOCAL = 3.14159265;

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_params.face_size || id.y >= g_params.face_size) { return; }

    float3 N     = cube_texel_to_direction(id.xy, g_params.face_index, float(g_params.face_size));
    float3 up    = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 right = normalize(cross(up, N));
    float3 fwd   = cross(N, right);

    float3 irradiance  = 0.0;
    float  sample_count = 0.0;
    float  delta = PI_LOCAL / 64.0;

    for (float phi = 0.0; phi < 2.0 * PI_LOCAL; phi += delta)
    {
        for (float theta = 0.0; theta < 0.5 * PI_LOCAL; theta += delta)
        {
            float3 L = sin(theta) * cos(phi) * right
                     + sin(theta) * sin(phi) * fwd
                     + cos(theta) * N;
            irradiance   += g_env_cube.SampleLevel(g_ibl_sampler, L, 0.0).rgb
                            * cos(theta) * sin(theta);
            sample_count += 1.0;
        }
    }

    g_output[id.xy] = float4(PI_LOCAL * irradiance / max(sample_count, 1e-4), 1.0);
}
