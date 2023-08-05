/* 
 * The main shader file. this file contains all shader stages and must be compiled
 * into the three shaders (vs, gs, ps).
 */

#include "functionsLib.hlsl"
#include "EngineConstants.h"

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

cbuffer SceneObjectsDataCBuffer : register(b1)
{
    int numObjects;
    int objectPolyCount[KS_MAX_SCENEOBJECTS];
    float4x4 objectTransforms[KS_MAX_SCENEOBJECTS];
};

//#define PROJECT_ORTHO

PixelInput vs_main(VertexInput input, uint vertID : SV_VertexID) {
    
    PixelInput output;
    
    // Projection:
    
#ifdef PROJECT_ORTHO
    // Debug ortho projection (top down):
    
    output.position = float4(input.position.x/5.0f, input.position.z/5.0f, 0.0f, 1.0f);
#else 
    // Step 1: Transform the vertex position to world space using the view matrix
    float4 worldPosition = mul(float4(input.position, 1.0f), viewMatrix);

    // Step 2: Transform the world space position to view space
    float4 viewPosition = mul(worldPosition, projectionMatrix);

    // Step 3: Set the output position (in clip space)
    output.position = viewPosition;
    
#endif
    return output;
}

StructuredBuffer<SurfaceLightmapDirectoryPacked> DirectoryBuffer : register(t0);
StructuredBuffer<SurfLight> SurfLightBuffer : register(t1);

#define SHADER_CONV_TESTd

#ifdef SHADER_CONV_TEST
#define SAMPLE_LARGE 10
#define SAMPLE_SCALE 15.0f
#define SAMPLE_STDEV 50.0f
#define SAMPLE_VAL   200.0f

float4 ps_main(PixelInput input, uint primID : SV_PrimitiveID) : SV_Target {
    // Perform convolution here
    // https://www.desmos.com/calculator/6wqxdr8v5k
    // sigma from -sampleLarge to +sampleLarge (two-dimensional)
    
    // This uses a simplified version, intead of convolving all previous steps, it only convolves
    // the very last step. For all previous steps simple visibility is used, similar to how radiosity
    // works. This is accurate enough for smaller polygons, but for larger ones it will be less accurate.
    // This is used because the convolution needs to be sampled, and it can be done fully either by caching
    // radiance at each bounce, which uses lots of vram, or samples in the pixel shader which would take
    // 10^(2*depth) samples per pixel, which is too slow. I fairly confident that the convolution is
    // solvable, so hopefully in the future no numerical methods will be needed.
    
    
    float2 sspos = input.position.xy;
    
    float2 a = float2(300.0f, 500.0f);
    float2 b = float2(670.0f, 500.0f);
    float2 c = float2(488.0f, 250.0f);
    float3 heights = float3(20.0f, 20.0f, 5.0f);
    float2 triMean = (a + b + c) / 3.0f;
    
    float meanHeight = 20.0f;
    float3 casterNormal = -normalize(cross(float3(b, heights[1]) - float3(a, heights[0]),
                                          float3(b, heights[1]) - float3(c, heights[2])
                          ));
    
    float color;
    
    float interDist;
    float interClip;
    float sum;
    
    // for now the stdev is just distance * 5 (this should be tuned as it depends on the shader)
    // also probably should follow inverse square law and not linear but idk if that applies
    // bcos gaussian
    float pixelstdev = 5.0f * barlerp(ComputeBarycentric(a, b, c, sspos), heights);

    for (int xt = -SAMPLE_LARGE; xt <= SAMPLE_LARGE; xt++) {
        for (int yt = -SAMPLE_LARGE; yt <= SAMPLE_LARGE; yt++) {
            
            interDist  = normaldist(sspos.x, pixelstdev, SAMPLE_SCALE * xt + triMean.x);
            interDist *= normaldist(sspos.y, pixelstdev, SAMPLE_SCALE * yt + triMean.y);
            interDist *= SAMPLE_VAL;
            
            interClip = isin(a, b, c, SAMPLE_SCALE * float2(xt, yt) + triMean);
            
            sum += interDist * interClip;
        }
    }
    
    
    // this was initially mean to be used for lambert but for now we just assume surfaces are omidirectional
    //float3 tri2frag = normalize(float3(sspos, 0) -
    //                            float3(clipIntoTri(a, b, c, sspos),
    //                                   barlerp(ComputeBarycentric(a, b, c, sspos), heights)));
    
    float3 tri2frag = normalize(float3(sspos, 0) -
                                float3(triMean,
                                       barlerp(ComputeBarycentric(a, b, c, triMean), heights)));
    
    // dont render anything behind the caster (we only account for light being cast in front of the caster)
    float behindmask = max(dot(tri2frag, casterNormal), 0) > 0 ? 1.0f : 0.0f;
    
    color = sum * behindmask;
    
    //float clipmask = isin(a, b, c, sspos) ? 1.0f : 0.0f;
    float clipmask = 0.0f;
    
    return float4(color + clipmask, color, color, 1.0f);
}
#else

float4 ps_main(PixelInput input, uint primID : SV_PrimitiveID) : SV_Target{
    SurfaceLightmapDirectoryPacked primDir = DirectoryBuffer[primID];
    int lightmapOffset = KS_MAX_SURFACE_LIGHTS * primID;
    SurfLight temp;
    
    if (primDir.numLights > KS_MAX_SURFACE_LIGHTS) {
        // this should never happen, we didnt cull the lightmap correctly
    }
    
    float3 color = primDir.color;
    
    if (primDir.emmisiveStrength < 0.1f) {
        color *= rand(primID)*0.2f + (1.0f - 0.5f);
    }
    //color *= primDir.emmisiveStrength;

    return float4(color, 1.0f);
}
#endif