struct FragmentInput
{
    [[vk::location(0)]] float4 color : COLOR0;
    float4 position : SV_Position;
};

float4 main(FragmentInput input) : SV_Target
{
    return float4(input.color);
}