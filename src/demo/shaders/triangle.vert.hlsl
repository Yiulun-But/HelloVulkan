struct VertexInput
{
    [[vk::location(0)]] float4 position : POSITION0;
    [[vk::location(1)]] float4 color : COLOR0;
};

struct VertexOutput
{
    [[vk::location(0)]] float4 color : COLOR0;
    float4 position : SV_Position;
};

struct UniformBuffer
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

[[vk::binding(0, 0)]] ConstantBuffer<UniformBuffer> ubo;

VertexOutput main(uint vertexId : SV_VertexID, VertexInput input)
{
    VertexOutput output;

    output.position = mul(ubo.proj, mul(ubo.view, mul(ubo.model, input.position)));
    output.color = input.color;

    return output;
}