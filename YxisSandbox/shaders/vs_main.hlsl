struct VSOutput
{
    float4 pos : SV_Position;
    float3 color : COLOR;
};

static const float2 positions[3] =
{
    float2(0.0f, -0.5f),
    float2(0.5f, 0.5f),
    float2(-0.5f, 0.5f)
};

static const float3 colors[3] =
{
    float3(1.0f, 0.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 0.0f, 1.0f),
};

VSOutput main(uint vertexIndex : SV_VertexID)
{
    VSOutput output;
    output.pos = float4(positions[vertexIndex], 0.0f, 1.0f);
    output.color = colors[vertexIndex];

    return output;
}