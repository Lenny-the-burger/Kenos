// Pragma once only works in C++ not HLSL :(
#ifdef __cplusplus
#pragma once
#endif

// This file holds the various constants used throughout the engine.
// they are all here so that they all stay consistent and can be changed easily.

// Default window size.
#define WINDOW_SIZE_W 1000
#define WINDOW_SIZE_H 800

// Maximum amount of times light could bounce.
#define KS_MAX_RAY_BOUNCES 4

// Maximum lights that can be rendered per surface. This only affects SSRDF buffer.
#define KS_MAX_SURFACE_LIGHTS 16

// Maximum polygons that can cast a shadow on a RDF. Since we bounce light between every surface
// we also have to store the shadows between every surface :(
#define KS_MAX_SHADOWS 64 // !! this affects SurfLight size, use either 4 or 7 !!

// Maximum number of objects that can shadow an RDF. The global index ranges are stored
// in the buffer, so shadows array should be 2 * KS_MAX_SHADOW_OBJS
#define KS_MAX_SHADOW_OBJS 6

// Constants for the convolution shader, see https://www.desmos.com/calculator/6wqxdr8v5k 
// for a graph of the shader function. TODO: make the graph better.

// Large number for convolution sampling. this results in large^2 samples per pixel.
#define KS_SAMPLE_LARGE 10

// Scale of the convolution sampling area. If you are seeing squares where triangles
// should be increase this.
#define KS_SAMPLE_SCALE 10.0f

// Maximum number of scene objects, this is used for cbuffer allocation.
// Maximum this can be: 962 (TODO: migrate either structuredbuffer or SRV)
#define KS_MAX_SCENEOBJECTS 20

// Minimum allowed lightness of a surface. Any RDFs with a lightness below this
// are forbidden from having children.
#define KS_MIN_LIGHTNESS 0.1f

// Side length of sample grid for shadows. Compute time expands KS_SHADOW_SAMPLES^2
#define KS_SHADOW_SAMPLES 10