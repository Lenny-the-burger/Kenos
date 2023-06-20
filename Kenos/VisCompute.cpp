#include "pch.h"
#include "VisCompute.h"

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

// Compute the VisArray from scratch
void recomputeVisArray(SceneInformation scene) {
	/* How visibility structure is used in diffuse compute :
	* 1. For each light source, or the current immedietly emissive set, get all of the visible
	*   polygons from the current source.
	* 2. Copy the RDF to all the visible ones, change some values (such as the mean path distance)
	*/

	/* How visibility structure is used in specular compute :
	* 1. For each light source, or the current immedietly emissive set, get the current focal point
	*   and reflect 180 degrees around a vector within the triangle to get the next focal point.
	* 2. Take the dot prod of the outgoing mean vector and the keys of the visibility array.
	* 3. By order of the dot product, compute weather the current RDF intersects with the reciever
	*	 polygon. If it does, copy the RDF to the reciever polygon and change some values.
	*
	* Down the pipeline specular shadows are also computed in basically the same way
	*/ 

	// Because of this the visibility structure can only cull things that we are absolutly sure
	// are not needed.
	
	/* How to compute visibility:
	* 1. For each polygon in the scene (need to be able to have a global index, so we can
	*	reference & iterate through all of the scene objects and still be able to go per polygon)
	* 2. Do backface culling of all the other triangles in the scene and the current one.
	* 3. Cull triangles based on if they are above or below the current one, we dont want
	*	triangles that are under the current one.
	* 4. For each triangle that is visible, compute the mean path distance between the current
	*	triangle and the visible one, which is the shortest path between the two. For now this is
	*	done by just getting the distance between the two triangle means, as triangles tend to be
	*	pretty small on modern models and not that visible. TODO: properly solve for the mean path
	* 5. Finally add the computed data to the visibility map in a FaceVisibilityData struct, where
	*	the key is the vector that goes from the caster to the reciever.
	* 
	* Note that steps 2 and 3 can be done in either order, will require more testing to find optimal
	* order. For now step 3 is done by dot prod of normal and vector to the other tri, but eventually
	* could use something like BSP as it essentially bisects the entire scene. Lots of potential
	* optimization to do here later on.
	*/

	// Clear the visibility array
	d_VisArray.clear();

	// Load the scene objects from the scene and set up the global index
	sceneObjects = scene.getSceneObjects();

	// count globalpolycount
	for (SceneObject& obj : sceneObjects) {
		globalPolyCount += (obj.GetMesh())->GetFaceCount();
	}
	
	// init stuff used in the loop so we arent constantly destroying and creating vars
	tuple<Vector3, Vector3, Vector3> currTri;
	tuple<Vector3, Vector3, Vector3> otherTri;

	Vector3 currNormal;
	Vector3 otherNormal;

	Vector3 currMean;
	Vector3 otherMean;
	
	float currMeanDist;
	
	Vector3 currToOther;

	// both of these have to be true for the other triangle ot be considered visible
	bool isBackfaceVisible;
	bool isBelowVisible;

	FaceVisibilityData currVisData;

	// the shortcut map, if triangle a is visible from triangle b then triangle b is visible from a
	// so no point in computing it twice.
	// adress it like this: shortcutMap[triangleA][triangleB] = true
	/*vector<map<int, bool>> shortcutMap;*/

	// for each polygon in the scene
	for (int globalIndex = 0; globalIndex < globalPolyCount; globalIndex++) {		
		currTri = getTribyGlobalIndex(globalIndex);
		currNormal = GetNormal(get<0>(currTri), get<1>(currTri), get<2>(currTri));
		currMean = (get<0>(currTri) + get<1>(currTri) + get<2>(currTri))/3;

		// add empty map to write vis data to
		d_VisArray.push_back(map<Vector3, FaceVisibilityData>());
		
		// initial visibility compute to create visibleIdxs
		for (int i = 0; i < globalPolyCount; i++) {
			// if the current polygon is the same as the one we are culling, skip it
			if (i == globalIndex) continue;

			// TODO: shortcut map
			//// check if it exists in the shortcutmap, if it does:
			//// if it is visible add it to the visibleIdxs
			//// if it is not visible, skip it
			//if (shortcutMap[globalIndex].count(i) > 0) {
			//	if (shortcutMap[globalIndex][i]) {
			//		// get the FaceVisibilityData from the previous entry and add it here
			//		otherMean = (get<0>(otherTri) + get<1>(otherTri) + get<2>(otherTri)) / 3;
			//		currToOther = currMean - otherMean; // its in this order because vector sub is backwards

			//		// get the vis data from the other triangle
			//		currVisData = d_VisArray[i][currToOther];
			//		
			//		
			//	}
			//	continue;
			//}

			// get the triangle we are evaluating
			otherTri = getTribyGlobalIndex(i);
			
			// get the other stuff we need
			otherNormal = GetNormal(get<0>(otherTri), get<1>(otherTri), get<2>(otherTri));
			otherMean = (get<0>(otherTri) + get<1>(otherTri) + get<2>(otherTri)) / 3;
			currToOther = currMean - otherMean; // its in this order because vector sub is backwards
			currMeanDist = currToOther.Length();

			// do culling
			isBackfaceVisible = currNormal.Dot(otherNormal) < 0; // need to face opposite dir
			isBelowVisible = currToOther.Dot(currNormal) > 0; // need to face the same dir

			// if the triangle is visible, add it to the visible array
			if (isBackfaceVisible && isBelowVisible) {
				// setup face vis data
				currVisData = FaceVisibilityData();
				currVisData.meanPathDist = currMeanDist;
				currVisData.receiverIdx = i;
				// implement dot target later
				
				d_VisArray[globalIndex][currToOther] = currVisData;
			}

			// add the result to the shortcut map
			/*shortcutMap[globalIndex][i] = isBackfaceVisible && isBelowVisible;*/
		}
	}
}
