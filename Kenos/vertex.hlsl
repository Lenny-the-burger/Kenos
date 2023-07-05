struct VertexInput
{
    float3 position : POSITION;
};

struct PixelInput
{
    float4 position : SV_Position;
};

PixelInput main(VertexInput input)
{
    PixelInput output;
    output.position = float4(input.position, 1.0f);
    return output;
}
