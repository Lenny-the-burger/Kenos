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
#define KS_MAX_SURFACE_LIGHTS 8

// Maximum polygons that can cast a shadow on a RDF. Since we bounce light between every surface
// we also have to store the shadows between every surface :(
#define KS_MAX_SHADOWS 6 // !! this affects SurfLight size, use either 4 or 7 !!

// Constants for the convolution shader, see https://www.desmos.com/calculator/6wqxdr8v5k 
// for a graph of the shader function. TODO: make the graph better.

#define KS_CONVOLUTION_SAMPLE_LARGE 5
#define KS_CONVOLUTION_SAMPLE_SCALE 0.5f

// Maximum number of scene objects, this is used for cbuffer allocation.
// Maximum this can be: 962 (TODO: migrate either structuredbuffer or SRV)
#define KS_MAX_SCENEOBJECTS 20