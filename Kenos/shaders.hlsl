struct VertexInput
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct PixelInput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

PixelInput vs_main(VertexInput input)
{
    PixelInput output;
    output.position = float4(input.position, 1.0f);
    output.color = input.color; // Write the 'color' semantic in the output
    return output;
}

float4 ps_main(PixelInput input) : SV_Target
{
    return float4(input.color, 1.0f);

}