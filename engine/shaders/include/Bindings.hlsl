// space0 — Bindless material resources (unbounded; Metal argument buffer Tier2)
Texture2D<float4>  g_material_textures[] : register(t0, space0);
SamplerState       g_material_samplers[] : register(s0, space0);

// space1 — Per-frame uniforms
struct DirectionalLight
{
    float3 direction;
    float  intensity;
    float3 color;
    float  _pad;
};

struct PointLight
{
    float3 position;
    float  radius;
    float3 color;
    float  intensity;
};

struct FrameUniforms
{
    float4x4         view_projection;
    float3           camera_world_pos;
    float            _pad0;
    DirectionalLight directional_lights[4];
    PointLight       point_lights[64];
    uint             num_directional_lights;
    uint             num_point_lights;
    uint2            _pad1;
};
ConstantBuffer<FrameUniforms> g_frame : register(b0, space1);

// space2 — IBL environment
TextureCube<float4> g_irradiance_cube : register(t0, space2);
TextureCube<float4> g_prefilter_cube  : register(t1, space2);
Texture2D<float>    g_brdf_lut        : register(t2, space2);
SamplerState        g_ibl_sampler     : register(s0, space2);

// space3 — Per-draw material indices (bindless array slot references)
struct MaterialIndices
{
    uint albedo_tex;
    uint normal_tex;
    uint roughness_tex;
    uint metallic_tex;
    uint ao_tex;
    uint emissive_tex;
};
ConstantBuffer<MaterialIndices> g_material : register(b0, space3);

// space27 — All-entity instance data (MSL buffer index = descriptor set = 27)
struct GPUInstanceData
{
    float4x4 world_matrix;
    uint     material_index;
    uint3    _pad;
};
StructuredBuffer<GPUInstanceData> g_instances       : register(t0, space27);

// space28 — GPU cull output: visible entity indices (MSL buffer index = descriptor set = 28)
StructuredBuffer<uint>            g_visible_indices  : register(t0, space28);
