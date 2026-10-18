struct FragmentInput
{
    [[vk::location(0)]] float3 color : COLOR0;
    [[vk::location(1)]] float2 texCoord : TEXCOORD0;
    
    float3 position : SV_Position;
};

[[vk::combinedImageSampler]]
[[vk::binding(1, 0)]]
Texture2D textureImage;

[[vk::combinedImageSampler]]
[[vk::binding(1, 0)]]
SamplerState textureSampler;

float4 main(FragmentInput input) : SV_Target
{
    return float4(textureImage.Sample(textureSampler, input.texCoord));
}