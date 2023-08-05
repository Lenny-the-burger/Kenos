/*
*   Contains common shader functions and structs
*/

#include "EngineConstants.h"

struct SurfLight {
    float3x3 bounds;

	// This is the corrected colour, so we dont have to sample the surface and mix it with the
	// light colour in the shader.
    float3 color;

	// The height of the mean of the verteces in the triangle. This is used at the end of the pixel
	// shader to do dot prod with the normal vector of the caster and the vector from the caster to
	// the pixel. This is used for the lambertian reflectance of the surface, if the caster
	// is completly isotropic (for example an emissive surface), this value could be ignored 
	// (TODO: implement this).

	// height above the plane of the vertex mean. To restore the original vertex mean do:
	// >> mean2pos = SV_POSITION - (boundMean + meanHeight * plane normal)
	// then multiply the result of the convolution by clamp(dot(mean2pos, casterNormal), 0, 1)
    float3 casterNormal;

    float stdev;

	// This stays the same through all bounces because of how the shader works maybe we can optimize
	// this but 4 extra bytes per structure isnt that bad. (we probably will have more padding anyway)
    float lightBrightness;

	// Shadows (triangles) cast from the parent. KS_MAX_SHADOWS
    float3x3 shadow[KS_MAX_SHADOWS];
    float3 shadowHeights[KS_MAX_SHADOWS];
    int numShadows;
    
    // 24 bytes of padding (4 bytes x 6)
    float4 padding;
    float2 padding2;
};

struct SurfaceLightmapDirectoryPacked {

	// This is the base colour of the surface.
    float3 color;

	// For now it either is or isnt emissive. Emssivie surfaces do not have lightmaps.
    int emmisiveStrength;

	// surflights will be held in their own buffer of size 
	// KS_MAX_SURFACE_LIGHTS * KS_MAX_RAY_BOUNCES * element amount * sizeof(SurfLight)
	// just use the current primitive id to index into the buffer with
	// KS_MAX_SURFACE_LIGHTS * KS_MAX_RAY_BOUNCES * primitiveID
	// Both of these structs will be held in a structured buffer so we dont have to worry about
	// getting size alignment right.

    int numLights;

	// not sure what else would go here, probaby stuff for deferred shading.
    
    float3 padding;
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
    float3 bary = ComputeBarycentric(a, b, c, p);
    return bary.x >= 0.0f && bary.y >= 0.0f && bary.z >= 0.0f;
}

float normaldist(float mean, float stdev, float x) {
    return (1 / (stdev * 2.50662827463)) * exp(-0.5f * pow((x - mean) / stdev, 2));
}

// Sample the given RDF at a given position
float3 sampleRDF(SurfLight RDF, float2 pos) {
    
    return float3(0.0f, 0.0f, 0.0f);
};

// Returns the shadow value at given pos with vertices abc and stdev
float convoluteShadow(float2 a, float2 b, float2 c, float stdev, float2 pos) {
    
    return 0.0f;
}

float rand(float seed) {
    return frac(sin(seed * seed) * 43758.5453123);
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

float barlerp(float3 bar, float3 vals) {
    return bar.x * vals.x + bar.y * vals.y + bar.z * vals.z;
}

// Convolve the given 
float convolute() {
    return 0;
}