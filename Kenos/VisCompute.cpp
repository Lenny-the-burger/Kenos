#include "pch.h"
#include "VisCompute.h"

#include "Mesh.h"
#include "Material.h"
#include "SceneObject.h"

#include "SceneInformation.h"

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
	// Index of reciever
	int recieverIdx;

	// Shortest distance between caster and reciever polygon (the mean path)
	float meanPathDist;

	// Indexes of all polygons that diffusly shadow the reciever from the caster
	// for specular we cannot precompute this because the focal point changes
	vector<int> diffuseShadowers;
};

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
	* 5. Compute the diffuse shadowers, which are the polygons that cast a shadow onto the current
	*	reciever from the caster. For now this is just done brute force by drawing from the pool
	*	of visible polygons that we computed in step 3. TODO: this will eventually use some sort of
	*	BVH or probably dot target culling.
	* 6. Finally add the computed data to the visibility map in a FaceVisibilityData struct, where
	*	the key is the vector that goes from the reciever to the caster.
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

    
}

// return triangle at global index idx
tuple<Vector3, Vector3, Vector3> getTribyGlobalIndex(int idx) {
	int currentSO = 0;
	int accTriCount = 0;
	for (SceneObject& obj : sceneObjects) {
		accTriCount += (obj.GetMesh())->GetFaceCount();
		
		// if the index is less than the accumulated tri count then we are in range
		if (idx < accTriCount) {
			// get the face index vector at position idx - accTriCount
			Vector3 faceIdxVec = (obj.GetMesh())->GetIndices()[idx - accTriCount];

			// using each element in the index vector construct tuple from GetFinalVtx()
			return make_tuple(
				obj.GetFinalVtx(faceIdxVec.x), 
				obj.GetFinalVtx(faceIdxVec.y), 
				obj.GetFinalVtx(faceIdxVec.z)
			);

		}

		// otherwise we need to move on to the next object
		currentSO++;
	}
	throw std::out_of_range("Triangle index out of range");
}

