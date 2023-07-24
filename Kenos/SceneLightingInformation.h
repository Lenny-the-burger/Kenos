#pragma once

#include <map>
#include <vector>

#include <DirectXMath.h>
#include <SimpleMath.h>

#include "SceneInformation.h"

using DXVector3 = DirectX::SimpleMath::Vector3;
using DXVector2 = DirectX::SimpleMath::Vector2;
using DXColor = DirectX::SimpleMath::Color;

// Maximum amount of times light could bounce.
#define KS_MAX_RAY_BOUNCES 4

// Maximum lights that can be rendered per surface. This only affects SSRDF buffer.
#define KS_MAX_SURFACE_LIGHTS 8

// Constants for the convolution shader, see https://www.desmos.com/calculator/6wqxdr8v5k 
// for a graph of the shader function. TODO: make the graph better.

#define KS_CONVOLUTION_SAMPLE_LARGE 5
#define KS_CONVOLUTION_SAMPLE_SCALE 0.5f

// The RDF that is traced throughout the scene. Contains the information needed to render the
// interaction of a caster surface on a reciever surface.
struct RadianceDistributionFunction {
	// The bounds of the RDF, used for rendering (RDF is stenciled to these bounds)
	std::tuple<DXVector3, DXVector3, DXVector3> bounds;

	// what bounce are we on?
	int bounce;

	// the stdev of the FRDF. this should be equivalent to the accumulated mean path dist
	float stdev;

	// Current colour (after being tinted by prev bounces)
	DXColor Color;

	// A map of all of the polygons that this RDF bounces on in the next bounce
	std::map<int, RadianceDistributionFunction> nextBounce;

	// accumulated shadow, this is merged and triangulated at some point
	std::vector<DXVector3> shadow;
};

// This is the same as the SSRDF, but contains information for lighting only at each light bounce.
struct LightPath {
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
	DXColor color; // for now we only store the final colour
	
	float stdev[KS_MAX_RAY_BOUNCES];

	std::tuple<DXVector3, DXVector3, DXVector3> bounds[KS_MAX_RAY_BOUNCES];

	// list of all the shadows that are cast on this SSRDF. This is triangulated
	// from the compound shadow of the RDF.
	std::vector<DXVector3> shadow[KS_MAX_RAY_BOUNCES];
};

// This is the RDF that scatters on the screen, or camera directly. All of the previous bounce steps
// are held in the LightPath array.
struct ScreeSpaceRDF {

	// this is the final vertices that will be drawn
	std::tuple<DXVector3, DXVector3, DXVector3> final_bounds;

	DXColor color;

	LightPath surf_lights[KS_MAX_SURFACE_LIGHTS];

	// not sure what else would go here, probaby stuff for deferred shading.
};

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

	// Returns the final vertex buffer to be rendered
	std::vector<ScreeSpaceRDF> GetFinalRDFBuffer();
	
	
private:
	SceneInformation& scene;
	
	float screenRatio;

	int globalPolyCount;

	// The lighting tree, a map of all of the indeces of the immedietly emissive polygons to the RDF
	// that is traced from them. This is not rendered directly.
	std::map<int, RadianceDistributionFunction> lightTree;

	// The actual vertex buffer that is render to screen. This is already converted from 3d space so
	// it can be rendered directly.
	std::vector<ScreeSpaceRDF> finalRDFbuffer;
};

