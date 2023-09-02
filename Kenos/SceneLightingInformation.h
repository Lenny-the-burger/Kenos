#pragma once

#include <map>
#include <vector>
#include <queue>
#include <algorithm>

#include <DirectXMath.h>
#include <SimpleMath.h>

#include "EngineConstants.h"

#include "SceneInformation.h"
#include "CoreFuncsLib.h"

using DXVector3 = DirectX::SimpleMath::Vector3;
using DXVector2 = DirectX::SimpleMath::Vector2;
using DXColor = DirectX::SimpleMath::Color;

/* These are used by the pixel shader :
* 1. Check if the current point is within the reflect bounds of that bounce, if not it is
*		0 for the current iteration
* 2. Start sum: -large to +large around centre of triangle
* 3. prev RDF * clip sample * gaussian (mean = sv_position, stdev = stdev, driver = sum sample)
*
* Repeat that for each bounce, on the first step the previous RDF is just 1. For now light brightness
* is controlled by a value in the gaussian sample, this may change in the future to the initial value
* in place of the first RDF.
*
*/

// Representaiton of the the radiance distribution function. This is what gets traced through the scene
// 
struct RDF {
	// Index of the SurfaceLightmapDirectory struct that this RDF belongs to.
	int parentDirectoryIndex;

	// Every time we process an rdf, if the bounce < maxBounces we spawn children
	int bounce;

	// When backtracing, index into the jumblemap of the parent RDF. If this is -1
	// then it has no parent and we are at a light source.
	int parentRDF;

	std::vector<int> children;

	// This is the corrected colour, so we dont have to sample the surface and mix it with the
	// light colour in the shader.
	DirectX::SimpleMath::Color color;

	// This stays the same through all bounces because of how the shader works maybe we can optimize
	// this but 4 extra bytes per structure isnt that bad. (we probably will have more padding anyway)
	float lightBrightness;

	// This is the per surface lightness multiplier. 1.0 is the default, that means that 100% of the
	// the light from the parent scatters onto the surface. 
	float lightness;

	// Indeces of objects that shadow this surface. Shadows are not stored per surface, they are stored
	// per mesh, the pixel shader iterates through all the provided ranges and checks. This should be
	// done in a better way.
	std::vector<int> shadows;
};

// A "directory" of all of the lightmap information for a surface. This is a per-surface structure
// that contains all of the information needed to render the surface. This is not the lightmap itself,
// as that is usually not per surface as light may bounce between surfaces any amount of times.
struct SurfaceLightmapDirectory {

	// This is the base colour of the surface.
	DirectX::SimpleMath::Color color;

	// Matrix that when multiplied with flattens geometry to the surface plane.
	DirectX::XMMATRIX flattenMatrix;

	DirectX::XMMATRIX toPlaneLocalMatrix;
	DirectX::XMMATRIX toWorldMatrix;	

	std::vector<int> surfLights;

	// vector of all of the surfaces that are visible from this surface.
	std::vector<int> visibleSurfaces;

	std::vector<int> visibleObjects;

	// not sure what else would go here, probaby stuff for deferred shading.
};

// shh padding i kno
// for now pad to 16 bytes to match hlsl, ideally it should be a multiple of 128 for cache alignment
// ig hlsl doesnt pad structs to 16 bytes
#pragma warning(disable: 4324)

// This is a version of the full RDF that gets stored in a buffer and actually used in rendering.
// Things not immedietly needed for rendering such as children, bounds, etc are not stored here.
// All of the vertex information has been flattened to the surface plane.
struct SurfLight {
	int casterIdx;

	// This is the corrected colour, so we dont have to sample the surface and mix it with the
	// light colour in the shader.
	DirectX::XMFLOAT3 color;

	// the mean path distance is calculated per fragment so we dont store it anymore

	// brightness of the caster light, this is computed for each bounce instead of iterativly
	// solving all previous RDFs in a light path. When we have a closed solution for the convolution
	// then we will solve the previous RDFs.
	float lightBrightness;

	// Shadows are for now stored and computed per fragment, i cant come up with a good
	// acceleration structure
};

// The same as SurfaceLightmapDirectory, but uses c style arrays to get ready to copy to buffers.
struct SurfaceLightmapDirectoryPacked {

	// Vertices of the surface. We are duplicating the vertex buffer, a better way is needed, 
	// probably just register the vertex buffer as a srv so the pixel shader can also acess it.
	DirectX::XMFLOAT3 vertices[3];

	// This is the base colour of the surface.
	DirectX::XMFLOAT3 color;

	// We alredy compute this so might as well hold on to it
	DirectX::XMFLOAT3 normal;

	// Emmisive strength of the surface. We can cull lightmaps that are overpowered by this.
	float emmissiveStrength;

	// number of lights that are on this surface (shouldnt more than KS_MAX_SURFACE_LIGHTS)
	int numLights;

	// Matrix that when multiplied with flattens geometry to the surface plane.
	DirectX::XMFLOAT4X4 flattenMatrix;

	// Converts vectors that are within the plane to axis aligned 2d and back.
	DirectX::XMFLOAT4X4 toPlaneLocalMatrix;
	DirectX::XMFLOAT4X4 toWorldMatrix;

	DirectX::XMFLOAT4 plane;
};

// Per surface (light agnostic):
// 1. Vertices
// 2. Normal
// 3. 3d plane space -> 2d axis algined tranform matrix
// 4. 2d axis aligned -> world space transform matrix (just inverse of above)
// 5. orthogonal projection matrix

// instead of actually sliding the verts along the normal, we can just slide the sample point
// as we already compute it for stdev. Then we can pretend that we slid the verts and the plane.

// Per light:
// 1. Light colour
// 2. Light brightness
// 3. surface source index

// enable padding warnings again
#pragma warning(default: 4324)

class SceneLightingInformation
{
public:
	SceneLightingInformation();
	SceneLightingInformation(SceneInformation& newScene);
	~SceneLightingInformation();
	
	void SetScene(SceneInformation& newScene);

	// set the screen ratio for rendering, this is used to stretch the x axis of the RDF
	// so input width/height.
	void SetScreenRatio(float ratio);

	// Rebuilds the entire light tree, this is usually only done at startup.
	void BuildLightTree();

	// Update the light tree, used when an emissive surface changes or when scene geometry changes
	// (such as a new model being added or animating). This recomputes the light tree using the given
	// model index as an entry point, deletes everything lower in the tree, and then recomputes the tree.
	void UpdateLightTree(int idx);

	// Constructs and flattens the final buffers. This has to be redone if you update the light tree.
	void UpdateFinalRDFBuffer();

	std::vector<SurfaceLightmapDirectoryPacked> GetDirectoryBuffer();
	std::map<int, std::vector<SurfLight>> GetFinalLightmapBuffer();
	
	
private:

	SceneInformation& scene;
	
	float screenRatio;

	int globalPolyCount;

	std::vector<DirectX::SimpleMath::Vector3> allNormals;

	std::vector<int> emissivePolygons;

	// The lighting tree, a vector of all of the light sources in the scene. Indexes into the jumbleMap
	// vector, each element is a member of the immedietly emissive set.
	std::vector<int> lightTree;

	// Contains all of the RDFs in the scene. It is called the jumble map because it is not sorted
	// or organized in any way, if you want to use this either enter from a directory or trace from
	// a lightTree root.
	std::vector<RDF> jumbleMap;

	std::vector<SurfaceLightmapDirectory> lightmapDirectories;

	std::vector<SurfaceLightmapDirectoryPacked> finalDirectoryBuffer;
	std::map<int, std::vector<SurfLight>> finalLightmapBuffer;

	// Queue of the 
	std::queue<int> processQueue;
};

