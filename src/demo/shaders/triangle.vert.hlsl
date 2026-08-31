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

VertexOutput main(uint vertexId : SV_VertexID, VertexInput input)
{
    VertexOutput output;

    output.position = input.position;
    output.color = input.color;

    return output;
}