#pragma once

#include <map>
#include <vector>

#include <DirectXMath.h>
#include <SimpleMath.h>

#include "SceneInformation.h"

using DXVector3 = DirectX::SimpleMath::Vector3;
using DXVector2 = DirectX::SimpleMath::Vector2;
using DXColor = DirectX::SimpleMath::Color;


// The RDF that is traced throughout the scene. Contains the information needed to render the
// interaction of a caster surface on a reciever surface.
struct RadianceDistributionFunction {
	// The bounds of the RDF, used for rendering (RDF is stenciled to these bounds)
	std::tuple<DXVector3, DXVector3, DXVector3> bounds;

	// what bounce are we on?
	int bounce;

	// the stdev of the FRDF (depends on mean path distance and if the current reflection is 
	// diffuse or specular). This is stored in a vector representing each step of the light path
	// as you cant represet 4 gaussian blurs with only one blur operation. This should be equivalent
	// in length to the bounce number.
	std::vector<float> stdev;

	// Current colour (after being tinted by prev bounces)
	DXColor Color;

	// A map of all of the polygons that this RDF bounces on in the next bounce
	std::map<int, RadianceDistributionFunction> nextBounce;
};

// Screen space version of the RDF, only used in final rendering
struct ScreeSpaceRDF {
	
	std::tuple<DXVector3, DXVector3, DXVector3> bounds;

	DXColor color;
	
	std::vector<float> stdev;

};

class SceneLightingInformation
{
public:
	SceneLightingInformation(SceneInformation& newScene);
	~SceneLightingInformation();
	
	void SetScene(SceneInformation& newScene);

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
	
	int globalPolyCount;

	// The lighting tree, a map of all of the indeces of the immedietly emissive polygons to the RDF
	// that is traced from them. This is not rendered directly.
	std::map<int, RadianceDistributionFunction> lightTree;

	// The actual vertex buffer that is render to screen. This is already converted from 3d space so
	// it can be rendered directly.
	std::vector<ScreeSpaceRDF> finalRDFbuffer;
};

