#pragma once

#include <string>
#include <map>

#include "Mesh.h"
#include "Material.h"
#include "SceneObject.h"

/*
* Contains all of the information for a scene. Usually used as a transition from scene.json to runtime
* and vise versa. This will eventually be used to save compute light tree and visibility data to disk.
*/

using DXVector3 = DirectX::SimpleMath::Vector3;

/* Camera setup like this:
*	Front view:
*     B---C
*	foc\A/oint
* 
*	Side view:
*       C
*		A-----focalPoint
* 
*	A is the screen position, usually treated as the camera position.
*	BC and unit vectors in the plane that makes up the screen
* 
*	The focal point is focal point focal length away and is on the normal of the screen plane
*	passing through a.
* 
*	Manually setting B or C or the focal point is not recomended, use the provided functions.
*/

struct Camera {
	DXVector3 Apos;
	DXVector3 Bpos;
	DXVector3 Cpos;
	
	DXVector3 focalPoint;
	float focalLength;

	// we need to store this to be able to recreate a new focal point
	DirectX::XMVECTOR prevRotQuat;
};

class SceneInformation
{
public:
	SceneInformation();
	SceneInformation(std::string sceneName);
	~SceneInformation();

	// Getters
	std::string getSceneName();
	std::string getSceneDescription();
	std::string getScenePath();

	std::map<std::string, Mesh> getSceneMeshes();
	std::map<std::string, Material> getSceneMaterials();
	
	std::vector<SceneObject>& getSceneObjects();

	int getGlobalPolyCount();

	// return triangle at global index idx
	std::tuple<DXVector3, DXVector3, DXVector3> getTribyGlobalIndex(int idx);

	// camera functions
	void setCameraPos(DXVector3 newPos);
	void rotateCamera(DXVector3 rotation);
	
	void setCameraFocalLength(float newFocalLength);
	float getCameraFocalLength();

	// Heres the camera object dumbass, this is what you get for not using builtins
	Camera getCam();

	DXVector3 untransformFromCam(DXVector3 vect);

private:
	// Scene object arrays
	std::map<std::string, Mesh> sceneMeshes;
	std::map<std::string, Material> sceneMaterials;
	
	// scene objects are the only ones not in a map because naming scene objects doesnt always make sence
	// this one is named "player" this one is a glossy chair and is named "bob"
	std::vector<SceneObject> sceneObjects;
	
	std::string sceneName;
	std::string sceneDescription;
	std::string scenePath;

	Camera cam;

	int globalPolyCount;
};