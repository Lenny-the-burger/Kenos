#pragma once

#include <string>
#include <map>

#include "EngineConstants.h"

#include "Mesh.h"
#include "Material.h"
#include "SceneObject.h"

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

	DXVector3 up;
	
	DXVector3 focalPoint;
	DXVector3 lookAt;
	float focalLength;

	// we need to store this to be able to recreate a new focal point
	DirectX::XMVECTOR prevRotQuat;

	// Store the view and projection matrices here
	DirectX::XMMATRIX viewMatrix;  // Store the view matrix here
	DirectX::XMMATRIX projectionMatrix;  // Store the projection matrix here
};

enum SceneSize
{
	// Small scene, allocate enough buffer to hold all the triangles
	KS_SCENESIZE_SMALL,
	// Medium scene, allocate enough buffer to hold half of the triangles, backface culling assumed
	// effective.
	KS_SCENESIZE_MEDIUM,
	// Large scene, allocate enough buffer to hold a quarter of the triangles, backface culling assumed
	// effective as well as frustum culling.
	KS_SCENESIZE_LARGE
};

/*
Contains all of the information for a scene. Usually used as a transition from scene.json to runtime
and vise versa. This will eventually be used to save compute light tree and visibility data to disk.
*/
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

	SceneSize getSceneSize();

	// return triangle at global index idx
	std::tuple<DXVector3, DXVector3, DXVector3> getTribyGlobalIndex(int idx);

	// reverse of getTribyGlobalIndex, returns the index of the object that contains 
	// the triangle at global index idx
	int getObjIndexbyGlobalIndex(int idx);

	// camera functions
	void setCameraPos(DXVector3 newPos);
	void rotateCamera(DXVector3 rotation);

	DirectX::XMMATRIX ComputeViewMatrix();
	DirectX::XMMATRIX ComputeProjectionMatrix(float newWidth, float newHeight);
	
	void setCameraFocalLength(float newFocalLength);
	float getCameraFocalLength();

	// Call this when the screen size changes. This is needed for the projection matrix
	void UpdateScreenSize(int newWidth, int newHeight);

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

	SceneSize size;

	int globalPolyCount;
};