#pragma pack_matrix(column_major)

struct FrameUniforms
{
    float4x4 view_projection;
    float3   camera_world_pos;
    float    _pad;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float2 world_xz : TEXCOORD0;
};

ConstantBuffer<FrameUniforms> g_frame : register(b0);

static const float k_extent = 1000.0f;

static const float3 k_verts[6] =
{
    float3(-k_extent, 0.0f, -k_extent),
    float3( k_extent, 0.0f, -k_extent),
    float3(-k_extent, 0.0f,  k_extent),
    float3(-k_extent, 0.0f,  k_extent),
    float3( k_extent, 0.0f, -k_extent),
    float3( k_extent, 0.0f,  k_extent),
};

VertexOut VSMain(uint vertex_id : SV_VertexID)
{
    float3 world_pos = k_verts[vertex_id];
    VertexOut output;
    output.position = mul(g_frame.view_projection, float4(world_pos, 1.0f));
    output.position.z += 0.0001f * output.position.w; // depth bias: ensure grid loses to coplanar geometry
    output.world_xz = world_pos.xz;
    return output;
}

float grid_alpha(float2 xz, float cell_size)
{
    float2 uv   = xz / cell_size;
    float2 grid = abs(frac(uv - 0.5f) - 0.5f) / fwidth(uv);
    return 1.0f - saturate(min(grid.x, grid.y));
}

float4 PSMain(VertexOut input) : SV_TARGET
{
    float minor = grid_alpha(input.world_xz, 1.0f);
    float major = grid_alpha(input.world_xz, 10.0f);
    float dist = length(input.world_xz - g_frame.camera_world_pos.xz);

    float fade_minor = 1.0f - smoothstep(15.0f, 50.0f,  dist);
    float fade_major = 1.0f - smoothstep(100.0f, 250.0f, dist);

    float alpha = saturate(minor * fade_minor * 0.4f + major * fade_major * 0.8f);

    return float4(0.8f, 0.8f, 0.8f, alpha);
}