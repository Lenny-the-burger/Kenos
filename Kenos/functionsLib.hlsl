/*
*   Contains common shader functions and structs
*/


// !! NOTE: This file, if modified, must be manualy recompiled (right click -> compile shaders.hlsl)
// because VS does not detect that the shader has to be recompiled because VS hlsl tools
// doest check for includes, thanks microsoft. !!

#include "EngineConstants.h"

struct SurfLight {
    int casterIdx;

	// This is the corrected colour, so we dont have to sample the surface and mix it with the
	// light colour in the shader.
    float3 color;

	// the mean path distance is calculated per fragment so we dont store it anymore

	// brightness of the caster light, this is computed for each bounce instead of iterativly
	// solving all previous RDFs in a light path. When we have a closed solution for the convolution
	// then we will solve the previous RDFs.
    float lightBrightness;
};

// The same as SurfaceLightmapDirectory, but uses c style arrays to get ready to copy to buffers.
struct SurfaceLightmapDirectoryPacked {
    // Vertices of the surface. We are duplicating the vertex buffer, a better way is needed, 
	// probably just register the vertex buffer as a srv so the pixel shader can also acess it.
    float3 vertices[3];

	// This is the base colour of the surface.
    float3 color;

	// We alredy compute this so might as well hold on to it
    float3 normal;

	// Emmisive strength of the surface. We can cull lightmaps that are overpowered by this.
    float emmissiveStrength;

	// number of lights that are on this surface (shouldnt more than KS_MAX_SURFACE_LIGHTS)
    int numLights;

	// Matrix that when multiplied with orthographically flattens geometry to the surface plane.
    float4x4 flattenMatrix;

	// Converts vectors that are within the plane to axis aligned 2d and back.
    float4x4 toPlaneLocalMatrix;
    float4x4 toWorldMatrix;
    
    // Coefficients of plane (a, b, c, d)
    float4 planeCoefficients;
};

struct Ray {
    float3 origin;
    float3 direction;
    float3 target;
    
    // TODO: this should be an immutable constructor
    
    void from(float3 newOrigin) {
        origin = newOrigin;
    }
    void to(float3 newTarget) {
        direction = normalize(newTarget - origin);
        target = newTarget;
    }
};

// Compute the barycentric coordinates of a point in a triangle
float3 ComputeBarycentric(float2 a, float2 b, float2 c, float2 p) {
    float2 v0 = b - a;
    float2 v1 = c - a;
    float2 v2 = p - a;

    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);

    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return float3(u, v, w);
}

bool isin(float2 a, float2 b, float2 c, float2 p) {
    float3 bar = ComputeBarycentric(a, b, c, p);
    return bar.x >= 0.0f && bar.y >= 0.0f && bar.z >= 0.0f;
}

bool isinbar(float3 bar) {
    return bar.x >= 0.0f && bar.y >= 0.0f && bar.z >= 0.0f;
}

float normaldist(float mean, float stdev, float x) {
    return (1 / (stdev * 2.50662827463)) * exp(-0.5f * pow((x - mean) / stdev, 2));
}

// Normalized normal distribution, used for shadows
float normalnormaldist(float mean, float stdev, float x) {
    return normaldist(mean, stdev, x)/normaldist(mean, stdev, mean);

}

// This might be wrong
float rand(float seed) {
    return frac(sin(seed * seed) * 43758.5453123);
}

// Generate a grid in baryocentric coordinates.
// size: side length size of grid
// x, y: euclidean grid coordinates in range 0->1
float3 barGrid(int size, float x, float y) {
    if (x + y <= 1) { // Easy case
        return float3(x, y, 1 - x - y);
    }
    float2 barSimple = float2(-x + 1 + (1.0f / 2.0f * size), -y + 1 + (1.0f / 2.0f * size));
    return float3(barSimple.x, barSimple.y, 1 - barSimple.x - barSimple.y);

}



// Helper function to get projection of p onto line segment ab
float2 projectOntoSegment(float2 a, float2 b, float2 p) {
    float2 ab = b - a;
    float t = dot(p - a, ab) / dot(ab, ab);
    return a + saturate(t) * ab;
}

//1. Check if the point to clip is within the triangle, if it is then just return the point.
//2. Project the point onto all three edges
//3. Check if the two closest projections (by angle) are on the edges, if neither are, return 
//   the closest vertex.
//4. Else divide the projections by weather they are on an edge, /1 if it is on /0 (+infty) if 
//   it is not. Return the closest one.
float2 clipIntoTri(float2 a, float2 b, float2 c, float2 p) {
    // Step 1: Check if the point to clip is within the triangle
    if (isin(a, b, c, p)) {
        return p;
    }

    // Project the point onto all three edges
    float2 abProjection = projectOntoSegment(a, b, p);
    float2 acProjection = projectOntoSegment(a, c, p);
    float2 bcProjection = projectOntoSegment(b, c, p);

    float2 projections[3] = { abProjection, acProjection, bcProjection };
    float distances[3] = { distance(abProjection, p), distance(acProjection, p), distance(bcProjection, p) };

    // Find the closest projection
    int minIndex = 0;
    for (int i = 1; i < 3; i++) {
        if (distances[i] < distances[minIndex]) {
            minIndex = i;
        }
    }

    return projections[minIndex];
}

float barlerp(float3 bar, float vals[3]) {
    return bar.x * vals[0] + bar.y * vals[1] + bar.z * vals[2];
}

float2 barlerp(float3 bar, float2 vals[3]) {
    return bar.x * vals[0] + bar.y * vals[1] + bar.z * vals[2];
}

float3 barlerp(float3 bar, float3 vals[3]) {
    return bar.x * vals[0] + bar.y * vals[1] + bar.z * vals[2];
}

// The main convolution function. This should be the main thing you optimize.
// Uses defined sampling constants from EngineConstants.h for everything (for now).
float convolute(float2 bounds[3], float dist, float value, float2 pos) {
    
    float2 a = bounds[0];
    float2 b = bounds[1];
    float2 c = bounds[2];
    float2 triMean = (a + b + c) / 3.0f;
    
    float interDist = 0.0f;
    float interClip = 0.0f;
    float sum = 0.0f;
    
    // for now the stdev is just distance * 5 (this should be tuned as it depends on the shader)
    // also probably should follow inverse square law and not linear but idk if that applies
    // bcos gaussian
    float pixelstdev = 1.0f * dist;

    for (int xt = -KS_SAMPLE_LARGE; xt <= KS_SAMPLE_LARGE; xt++) {
        for (int yt = -KS_SAMPLE_LARGE; yt <= KS_SAMPLE_LARGE; yt++) {
            
            interDist =  normaldist(pos.x, pixelstdev, KS_SAMPLE_SCALE * xt + triMean.x);
            interDist *= normaldist(pos.y, pixelstdev, KS_SAMPLE_SCALE * yt + triMean.y);
            interDist *= value;
            
            interClip = isin(a, b, c, KS_SAMPLE_SCALE * float2(xt, yt) + triMean);
            
            sum += interDist * interClip;
        }
    }
    
    
    // this was initially mean to be used for lambert but for now we just assume surfaces are omidirectional
    //float3 tri2frag = normalize(float3(sspos, 0) -
    //                            float3(clipIntoTri(a, b, c, sspos),
    //                                   barlerp(ComputeBarycentric(a, b, c, sspos), heights)));
    
    //float3 tri2frag = normalize(float3(pos, 0) -
    //                            float3(triMean, barlerp(ComputeBarycentric(a, b, c, triMean), heights)));
    
    // dont render anything behind the caster (we only account for light being cast in front of the caster)
    //float behindmask = max(dot(tri2frag, casterNormal), 0) > 0 ? 1.0f : 0.0f;
    
    return sum;
}

// The same as regular convolution but for shadows. Instead of additive, this returns a value
// from a normalized distribution so you have to multiply the result by the light intensity.
float convoluteShadow(float2 bounds[3], float dist, float2 pos) {
    // Basically exactly the same as convolute() but with a different sampling function,
    // no value, and the stdev is lerped between the bounds during convolution.
    float2 a = bounds[0];
    float2 b = bounds[1];
    float2 c = bounds[2];
    float2 triMean = (a + b + c) / 3.0f;
    
    float interDist = 0.0f;
    float interClip = 0.0f;
    float sum = 0.0f;
    
    float2 samplePos;
    float3 samplePosBar;

    for (int xt = -KS_SAMPLE_LARGE; xt <= KS_SAMPLE_LARGE; xt++) {
        for (int yt = -KS_SAMPLE_LARGE; yt <= KS_SAMPLE_LARGE; yt++) {
            samplePos = float2(KS_SAMPLE_SCALE * xt + triMean.x, KS_SAMPLE_SCALE * yt + triMean.y);
            
            samplePosBar = ComputeBarycentric(a, b, c, samplePos);
            
            interDist =  normalnormaldist(pos.x, dist, samplePos.x);
            interDist *= normalnormaldist(pos.y, dist, samplePos.y);
            
            interClip = isin(a, b, c, KS_SAMPLE_SCALE * float2(xt, yt) + triMean);
            
            sum += interDist * interClip;
        }
    }
    
    return sum;
}

// Compute baryocnetric coordinates of ray-triangle intersection
float4 intersect(Ray ray, float3 v0, float3 v1, float3 v2) {
    float3 o = ray.origin;
    float3 d = ray.direction;
    
    // Edges of the triangle
    float3 e1 = v0 - v2;
    float3 e2 = v1 - v2;

    // Vector from a vertex of the triangle to the ray origin
    float3 t = o - v2;

    // Compute perpendicuar vectors p and q using cross products
    float3 p = cross(d, e2);
    float3 q = cross(t, e1);

    

    // Calculate the barycentric coordinates a using dot products
    float3 a = float3(dot(q, e2), dot(p, t), dot(q, d)) / dot(p, e1);

    // Return barycentric coordinates and complementary value
    return float4(a, 1.0 - a.y - a.z);
}

float4x4 MatrixShadow(float4 ShadowPlane, float4 LightPosition) {
    float4 P = normalize(ShadowPlane);
    float Dot = dot(P, LightPosition);

    P = -P;
    float D = P.w;
    float C = P.z;
    float B = P.y;
    float A = P.x;

    // init matrix to 0
    float4x4 M = float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    M._44 = Dot + D * LightPosition.w; // r[3].w
    M._43 = D * LightPosition.z; // r[3].z
    M._42 = D * LightPosition.y; // r[3].y
    M._41 = D * LightPosition.x; // r[3].x

    Dot -= D * LightPosition.w;

    M._34 = Dot + C * LightPosition.w; // r[2].w
    M._33 = C * LightPosition.z + 1; // r[2].z
    M._32 = C * LightPosition.y; // r[2].y
    M._31 = C * LightPosition.x; // r[2].x

    Dot -= C * LightPosition.z;

    M._24 = Dot + B * LightPosition.w; // r[1].w
    M._23 = B * LightPosition.z; // r[1].z
    M._22 = B * LightPosition.y + 1; // r[1].y
    M._21 = B * LightPosition.x; // r[1].x

    Dot -= B * LightPosition.y;

    M._14 = Dot + A * LightPosition.w; // r[0].w
    M._13 = A * LightPosition.z; // r[0].z
    M._12 = A * LightPosition.y; // r[0].y
    M._11 = A * LightPosition.x + 1; // r[0].x

    return M;
}

float PointInOrOn(float3 P1, float3 P2, float3 A, float3 B)
{
    float3 CP1 = cross(B - A, P1 - A);

    float3 CP2 = cross(B - A, P2 - A);

    return 1 - step(0.0, dot(CP1, CP2));
}

bool PointInTriangle(float3 px, float3 p0, float3 p1, float3 p2)
{
    return
        PointInOrOn(px, p0, p1, p2) *
        PointInOrOn(px, p1, p2, p0) *
        PointInOrOn(px, p2, p0, p1);
}

float3 IntersectPlane(Ray ray, float3 p0, float3 p1, float3 p2)
{
    float3 D = ray.direction;
    float3 N = cross(p1 - p0, p2 - p0);
    float3 X = ray.origin + D * dot(p0 - ray.origin, N) / dot(D, N);

    return X;
}

bool IntersectTriangle(Ray ray, float3 p0, float3 p1, float3 p2)
{
    float3 X = IntersectPlane(ray, p0, p1, p2);
    return PointInTriangle(X, p0, p1, p2);
}

float3 IntersectTriangle2(Ray ray, float3 p0, float3 p1, float3 p2)
{
    float3 X = IntersectPlane(ray, p0, p1, p2);
    if (PointInTriangle(X, p0, p1, p2)) {
        return X;
    }
    else{
        return X * +1.#INF;
    }
}