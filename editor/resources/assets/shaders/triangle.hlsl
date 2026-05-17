struct SceneUniforms
{
    float4x4 transform;
};

struct VertexIn
{
    float3 position : POSITION;
    float4 color    : COLOR;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

ConstantBuffer<SceneUniforms> g_uniforms : register(b1);

VertexOut VSMain(VertexIn input)
{
    VertexOut output;
    output.position = mul(g_uniforms.transform, float4(input.position, 1.0));
    output.color    = input.color;
    return output;
}

float4 PSMain(VertexOut input) : SV_TARGET
{
    return input.color;
}
