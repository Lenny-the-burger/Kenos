#include "pch.h"
#include "VisCompute.h"

#include <map>
#include <vector>

using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;

/* Compute visibilty information.
*  
* Diffuse VisArray (d_VisArray):
*	An array of hashmaps that contain visibility information for each face in the
*	following format: {vector from caster polygon to each reciever polygon:reciever polygon idx}
* 
*	It is stored in thos format so that it can be acessed easier when performing specular reflections
*	by just itertevly guessing which polygon will be hit based on the dot product of the outgoing
*	light vector and the keys of the hashmap.
* 
* Face visibility data array (d_FaceVisData):
*	An array of hashmaps that contain visibility information for each face in the
*	following format: {idx of visible face:FaceVisibilityData}. This structure
*	contains data used for lighting tree construction, such as the mean path distance
*	and more (see structure below)
*/

// Struct for storing visibility data, holds shadows, mean path distance
struct FaceVisibilityData {
	// Vector to reflect along for specular
	Vector3 reflectVector;

	// Shortest distance between caster and reciever polygon (the mean path)
	float meanPathDist;

	// Indexes of all polygons that shadow the reciever from the caster
	vector<int> shadowers;
};

// Compute the reflect array, just go through each triangle and find one that lies
// on the same plane and is within the triangle. Reflections will only ever eb 180 degrees
