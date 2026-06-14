float radical_inverse_vdc(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), radical_inverse_vdc(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a2        = roughness * roughness * roughness * roughness;
    float phi       = 2.0 * 3.14159265 * Xi.x;
    float cos_theta = sqrt((1.0 - Xi.y) / max(1.0 + (a2 - 1.0) * Xi.y, 1e-4));
    float sin_theta = sqrt(max(0.0, 1.0 - cos_theta * cos_theta));
    float3 H        = float3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
    float3 up       = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 right    = normalize(cross(up, N));
    float3 fwd      = cross(N, right);
    return normalize(right * H.x + fwd * H.y + N * H.z);
}

// Vulkan spec §16.7.8: sc/tc are the face's s/t major axes; s = sc/ma, t = tc/ma.
// Inverted per face (given normalised s,t → 3D direction where major axis = ±1):
//   +X: dir = ( 1, -t, -s)    -X: dir = (-1, -t,  s)
//   +Y: dir = ( s,  1,  t)    -Y: dir = ( s, -1, -t)
//   +Z: dir = ( s, -t,  1)    -Z: dir = (-s, -t, -1)
float3 cube_texel_to_direction(uint2 texel, uint face, float size)
{
    float2 uv = (float2(texel) + 0.5) / size;
    float  s  = uv.x * 2.0 - 1.0;
    float  t  = uv.y * 2.0 - 1.0;
    float3 dir;
    switch (face)
    {
        case 0: dir = float3( 1.0, -t, -s); break;
        case 1: dir = float3(-1.0, -t,  s); break;
        case 2: dir = float3(   s,  1,  t); break;
        case 3: dir = float3(   s, -1, -t); break;
        case 4: dir = float3(   s, -t,  1); break;
        default: dir = float3(  -s, -t, -1); break;
    }
    return normalize(dir);
}
