struct VertexInput
{
    float3 position : POSITION;
};

struct PixelInput
{
    float4 position : SV_Position;
};

cbuffer CONSTANT_BUFFER_STRUCT : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    int sampleLarge;
    float sampleScale;
};

#define PROJECT_ORTHOn

PixelInput vs_main(VertexInput input)
{
    PixelInput output;
    
#ifdef PROJECT_ORTHO
    // Debug ortho projection (top down):
    
    output.position = float4(input.position.x/5.0f, input.position.z/5.0f, 0.0f, 1.0f);
#else 
    // Step 1: Transform the vertex position to world space using the view matrix
    float4 worldPosition = mul(float4(input.position, 1.0f), viewMatrix);

    // Step 2: Transform the world space position to view space
    float4 viewPosition = mul(worldPosition, projectionMatrix);

    // Step 3: Set the output position (in clip space)
    //output.position = (viewPosition * float4(0.5, 0.5, -0.01, 1)) + float4(0, 1, 0, 0);
    output.position = viewPosition;
    
#endif
    return output;
}

float4 ps_main(PixelInput input, uint primID : SV_PrimitiveID) : SV_Target
{
    // Perform convolution here
    // https://www.desmos.com/calculator/6wqxdr8v5k
    // sigma from -sampleLarge to +sampleLarge (two-dimensional)
    
    //float color = (float) primID / 368;
    float color;
    
    color = (float) primID / 720.0f;
    
    return float4(color, color, color, 1.0f);
}
