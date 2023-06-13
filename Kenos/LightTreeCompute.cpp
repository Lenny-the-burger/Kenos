#include "pch.h"
#include "LightTreeCompute.h"

#include <map>
#include <vector>

using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;

/* Compute the lighting tree.
* 
* This computes the lighting tree, which holds all of the lighting information for a scene.
* This is done by tracing polygons through the scene from light sources or emissive surfaces,
* although point lights are treated as "reflection 0" as this model assumes that light can
* only come from and scatter onto polygons (triangle). 
* 
* 1. Start at a light in the scene and using the visarray acceleration structure determine
*	what other polygons are visible from the current source with specular and diffuse reflections.
*	For diffuse refections, the full visibility map is used, but for specular reflectiong, only
*	direct reflections are allowed.
* 
*	This works because reflections can be seperated into perfectly diffuse and specular (at least
*	the ones that are important to this model), and each computed mostly independently. For example
*	if a surface has 0.5 roughness it does not mean that the surface is a mirror but with less
*	parralax, but that 50% of the light is reflected diffusly and 50% is reflected specularly.
*	This is a limitatin of the currently accepted PBR model, but more esoteric BRDFs are usually
*	not necessary anyway.
* 
* 2. compute the distance 
*/

// The RDF that is traced throughout the scene. Contains the information needed to render the
// interaction of a caster surface on a reciever surface.
struct RadianceDistributionFunction {
	// The bounds of the RDF, used for rendering (RDF is stenciled to these bounds)
	tuple<Vector3, Vector3, Vector3> bounds;
	
	// what bounce are we on?
	int bounce;
	
	// the stdev of the FRDF (depends on mean path distance and if the current reflection is 
	// diffuse or specular). This is stored in a vector representing each step of the light path
	// as you cant represet 4 gaussian blurs with only one blur operation. This should be equivalent
	// in length to the bounce number.
	vector<float> stdev;
	
	// Current colour (after being tinted by prev bounces)
	Color color;
	
	// Original emissive colour of the surface that this RDF is being traced from
	Color emissiveColor;
	
	// A map of all of the polygons that this RDF bounces on in the next bounce
	map<int, RadianceDistributionFunction> nextBounce;
};

// This isnt optimized for now, we just trace every vertex in the scene instead of lumping some
// together as they are on models, but this is a good starting point

// The lighting tree, a map of all of the indeces of the immedietly emissive polygons to the RDF
// that is traced from them. This is not rendered directly.
map<int, RadianceDistributionFunction> lightTree;

// The actual vertex buffer that is render to screen. This is already converted from 3d space so
// it can be rendered directly.
vector<tuple<Vector2, Vector2, Vector2>> finalVertexBuffer;
vector<RadianceDistributionFunction> finalRDFbuffer;

// Rebuilds the entire light tree, this is usually only done at startup.
void buildLightTree() {

};

// Update the light tree, used when an emissive surface changes or when scene geometry changes
// (such as a new model being added or animating). This recomputes the light tree using the given
// model index as an entry point, deletes everything lower in the tree, and then recomputes the tree.
// 
// This will eventually take in discreete object -> polygon indexes, but for now thid engine does not
// support animation so there is no point for now.
//
// Parameters:
//		idx: The index of the emissive surface (sceneobject) that is being updated
void updateLightTree(int idx) {
	
}