#pragma once

#include <map>
#include <vector>

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

	// When backtracing in the shader, index into the lightmap of the parent RDF. If this is nullptr
	// then it has no parent and we are at a light source.
	RDF* parentRDF;

	std::vector<RDF> children;

	// This is the corrected colour, so we dont have to sample the surface and mix it with the
	// light colour in the shader.
	DirectX::SimpleMath::Color color;

	float stdev;

	// This stays the same through all bounces because of how the shader works maybe we can optimize
	// this but 4 extra bytes per structure isnt that bad. (we probably will have more padding anyway)
	float lightBrightness;

	// Shadows (triangles) cast from the parent.
	std::vector<DirectX::XMFLOAT3X3> shadow;
	std::vector<float> shadowStdev;
};

// A "directory" of all of the lightmap information for a surface. This is a per-surface structure
// that contains all of the information needed to render the surface. This is not the lightmap itself,
// as that is usually not per surface as light may bounce between surfaces any amount of times.
struct SurfaceLightmapDirectory {

	// This is the base colour of the surface.
	DirectX::SimpleMath::Color color;

	// Matrix that when multiplied with flattens geometry to the surface plane.
	DirectX::XMMATRIX flattenMatrix;

	std::vector<RDF> surfLights;

	// not sure what else would go here, probaby stuff for deferred shading.
};

// shh padding i kno
#pragma warning(disable: 4324)

// This is a version of the full RDF that gets stored in a buffer and actually used in rendering.
// Things not immedietly needed for rendering such as children, bounds, etc are not stored here.
// All of the vertex information has been flattened to the surface plane.
struct alignas(128) SurfLight {
	DirectX::XMFLOAT3X3 bounds;

	// This is the corrected colour, so we dont have to sample the surface and mix it with the
	// light colour in the shader.
	DirectX::XMFLOAT3 color;

	DirectX::XMFLOAT3 casterNormal;

	// this is the total from the path. computed as sqrt(sum of squares of each stdev)
	float stdev;

	// This stays the same through all bounces because of how the shader works maybe we can optimize
	// this but 4 extra bytes per structure isnt that bad. (we probably will have more padding anyway)
	float lightBrightness;

	// Shadows (triangles) cast from the parent. KS_MAX_SHADOWS
	DirectX::XMFLOAT3X3 shadow[KS_MAX_SHADOWS];
	DirectX::XMFLOAT3   shadowHeights[KS_MAX_SHADOWS];
	int numShadows;
};

// The same as SurfaceLightmapDirectory, but uses c style arrays to get ready to copy to buffers.
struct alignas(32) SurfaceLightmapDirectoryPacked {

	// This is the base colour of the surface.
	DirectX::XMFLOAT3 color;

	// Emmisive strength of the surface. We can cull lightmaps that are overpowered by this.
	int emmissiveStrength;

	// surflights will be held in their own buffer of size 
	// KS_MAX_SURFACE_LIGHTS * KS_MAX_RAY_BOUNCES * element amount * sizeof(SurfLight)
	// just use the current primitive id to index into the buffer with
	// KS_MAX_SURFACE_LIGHTS * KS_MAX_RAY_BOUNCES * primitiveID
	// Both of these structs will be held in a structured buffer so we dont have to worry about
	// getting size alignment right.

	int numLights;

	// not sure what else would go here, probaby stuff for deferred shading.
};

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

	// Constructs the final RDF buffer to be rendered
	void UpdateFinalRDFBuffer();

	std::vector<SurfaceLightmapDirectoryPacked> GetDirectoryBuffer();
	std::vector<SurfLight> GetFinalLightmapBuffer();
	
	
private:
	void MurderRDFfamilly(RDF& parent);

	SceneInformation& scene;
	
	float screenRatio;

	int globalPolyCount;

	// The lighting tree, a map of all of the indeces of the immedietly emissive polygons to the RDF
	// that is traced from them. This is not rendered directly.
	std::map<int, RDF> lightTree;

	std::vector<SurfaceLightmapDirectoryPacked> finalDirectoryBuffer;
	std::vector<SurfLight> finalLightmapBuffer;
};

