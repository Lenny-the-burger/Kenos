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
    float3 WorldPosition : POSITION;
};

cbuffer CONSTANT_BUFFER_STRUCT : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    
    int sampleLarge;
    float sampleScale;
    
    int screenW;
    int screenH;
    
    int globalPolyCount;
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
    output.WorldPosition = input.position;
    
#endif
    return output;
}

StructuredBuffer<SurfaceLightmapDirectoryPacked> DirectoryBuffer : register(t0);
StructuredBuffer<SurfLight> SurfLightBuffer : register(t1);

// For now we will view tranform the lightmap in the pixel shader. This should ideally be done
// in either the geometry shader or another compute shader that runs every frame.
// keep in mind that this involve literally only two matrix multilications so its not that expensive

#define SHADER_CONV_TESTf

#ifdef SHADER_CONV_TEST

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
    
    float2 tri[3] = {a, b, c };
    
    float color = convolute(tri, 1, SAMPLE_VAL, sspos);
    
    return float4(color, color, color, 1.0f);
}
#else

float4 ps_main(PixelInput input, uint primID : SV_PrimitiveID) : SV_Target{
    SurfaceLightmapDirectoryPacked r_dir = DirectoryBuffer[primID];
    int lightmapOffset = KS_MAX_SURFACE_LIGHTS * primID;
    
    SurfLight light = SurfLightBuffer[lightmapOffset];
    
    if (r_dir.numLights > KS_MAX_SURFACE_LIGHTS) {
        // this should never happen, we didnt cull the lightmap correctly
    }
    
    float2 sspos = input.position.xy;
    float3 worldPos = input.WorldPosition;
    
    float3 color = r_dir.color;
    float brightness = 0.0f;
    
    float3 r_norm = r_dir.normal;
    float3 r_tri[3];
    r_tri = r_dir.vertices;
    
    float3 r_mean = r_tri[0] + r_tri[1] + r_tri[2];
    r_mean /= 3.0f;
    
    //brightness += light.casterIdx;
    /*
    for (int i = 0; i < r_dir.numLights; i++) {
        
        SurfLight light = SurfLightBuffer[lightmapOffset + i];
        SurfaceLightmapDirectoryPacked c_dir = DirectoryBuffer[light.casterIdx];
        float3 posFlattened = mul(float4(worldPos, 1.0f), c_dir.flattenMatrix).xyz;
        
        float fragDist = distance(posFlattened, worldPos);
        
        float2 fragFlatLocal = mul(float4(posFlattened, 1.0f), c_dir.toPlaneLocalMatrix).xy;
        
        float3 c_tri[3];
        c_tri = c_dir.vertices;
        
        float2 c_triLocal[3];
        c_triLocal[0] = mul(float4(c_tri[0], 1.0f), c_dir.toPlaneLocalMatrix).xy;
        c_triLocal[1] = mul(float4(c_tri[1], 1.0f), c_dir.toPlaneLocalMatrix).xy;
        c_triLocal[2] = mul(float4(c_tri[2], 1.0f), c_dir.toPlaneLocalMatrix).xy;

        float2 c_mean_local = c_triLocal[0] + c_triLocal[1] + c_triLocal[2];
        c_mean_local /= 3.0f;
        
        float lightBrightness = convolute(c_triLocal, fragDist, 10.0f, fragFlatLocal);
        //float lightBrightness = isin(tri[0], tri[1], tri[2], fragFlatLocal);

        
        float3 c_mean = c_tri[0] + c_tri[1] + c_tri[2];
        c_mean /= 3.0f;
        
        float3 c_norm = c_dir.normal;
        
        float shadowProduct = 1.0f;
        
        
        
        float4x4 shadowMat = MatrixShadow(c_dir.planeCoefficients, float4(worldPos, 1.0f));
        
        
        
        
        
        
        brightness += 1.0f * shadowProduct;
        
    }
    */
    
    // for now just blow out emissive
    if (r_dir.emmissiveStrength > 0.1f) {
        brightness = 1.0f;
    }
    
    SurfaceLightmapDirectoryPacked s_dir;
    Ray shadowRay = (Ray) 0;
    shadowRay.from(float3(0, 0, 0));
    shadowRay.to(worldPos);

    float fragDepth = +1.#INF;
    float minDepth = distance(worldPos, float3(0, 0, 0));
    
    for (int s_idx = 0; s_idx < globalPolyCount; s_idx++) {
        s_dir = DirectoryBuffer[s_idx];
        float3 s_tri[3] = s_dir.vertices;
            
            //// method 1
            //float4 interBar = intersect(shadowRay, s_tri[0], s_tri[1], s_tri[2]);
            //if (isinbar(interBar.xyz)) {
            //    shadowProduct = 0.0f;
            //    break;
            //}
            
            //// method 2
            //if (IntersectTriangle(shadowRay, s_tri[0], s_tri[1], s_tri[2])) {
            //    shadowProduct = 0.0f;
            //    break;
            //}
            
        float3 inter = IntersectTriangle2(shadowRay, s_tri[0], s_tri[1], s_tri[2]);
        float newDepth = distance(inter, float3(0, 0, 0));
            
            // We found an occludder
        if (newDepth < minDepth) {
            fragDepth = 0;
            break;
        }
            
        fragDepth = min(fragDepth, newDepth);
    }
    
    float luminocity = fragDepth;
    
    //return float4(color * brightness, 1.0f);
    return float4(luminocity, luminocity, luminocity, 1.0f);
    //return float4(luminocity, 1.0f);
    //return float4(ss_bounds[0].x / 1000.0f, ss_bounds[0].y / 800.0f, 0.0f, 1.0f);
    //return float4(input.WorldPosition/4.0f, 1.0f);

}
#endif