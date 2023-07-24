#include "pch.h"

#include "SceneInformation.h"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;

// Default constructor
SceneInformation::SceneInformation() {
	globalPolyCount = 0;
	size = KS_SCENESIZE_SMALL;
};

SceneInformation::SceneInformation(string filePath) {
	// Load the scene file
	ifstream f(filePath);
	if (!f.good()) {
		string errorMessage = "Scene file file '" + filePath + "' does not exist!";
		MessageBoxA(NULL, errorMessage.c_str(), "Fatal error", MB_ICONERROR | MB_OK);
		exit(0);
	}

	json data = json::parse(f);

	// Get the basic strings
	sceneName = data["sceneName"];
	sceneDescription = data["sceneDescription"];
	scenePath = filePath;
	
	// Create materials
	for (auto& material : data["materials"]) {
		Material m;
		m.SetRoughness(material["roughness"]);
		m.SetAlbedo(Color(material["albedo"][0], material["albedo"][1], material["albedo"][2]));
		m.SetEmissiveIntensity(material["emissiveIntensity"]);
		
		sceneMaterials[material["name"]] = m;
	}

	// Create meshes
	// scan through all of the meshes listed in the "meshes" array and throw error if cannot be found
	for (string mesh : data["meshes"]) {
		// check if the mesh file exists
		ifstream f(mesh);
		if (!f.good()) {
			string errorMessage = "Mesh file '" + mesh + "' does not exist!";
			MessageBoxA(NULL, errorMessage.c_str(), "Fatal error", MB_ICONERROR | MB_OK);
			exit(0);
		}

		// use AssImp to import mesh
		
		Assimp::Importer importer;
		
		const aiScene* scene = importer.ReadFile(mesh, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

		// check if the mesh file is valid
		if (!scene) {
			string errorMessage = "Mesh file '" + mesh + "' is invalid (AssImp error)!";
			MessageBoxA(NULL, errorMessage.c_str(), "Fatal error", MB_ICONERROR | MB_OK);
			exit(0);
		}

		// get the first mesh in the scene
		aiMesh* aiMesh = scene->mMeshes[0];
		
		// create a vector to store the vertices
		vector<Vector3> verts;
		// create a vector to store the faces (indices)
		vector<Vector3> faces;

		// scan through all of the vertices in the mesh
		for (int i = 0; i < aiMesh->mNumVertices; i++) {
			// get the vertex position
			aiVector3D aiPos = aiMesh->mVertices[i];
			// convert the vertex position to a Vector3
			Vector3 pos = Vector3(aiPos.x, aiPos.y, aiPos.z);
			// add the vertex position to the vector of vertices
			verts.push_back(pos);
		}

		// scan through all of the faces in the mesh
		for (int i = 0; i < aiMesh->mNumFaces; i++) {
			// get the face
			aiFace aiFace = aiMesh->mFaces[i];
			
			// get each index from the face and write to a Vector3
			Vector3 face = Vector3(aiFace.mIndices[0], aiFace.mIndices[1], aiFace.mIndices[2]);

			// add the face to the vector of faces
			faces.push_back(face);
		}

		//Add mesh to the list of meshes in the scene
		sceneMeshes[mesh] = Mesh{ verts, faces };
		
	}

	// Create scene objects
	for (auto& object : data["objects"]) {
		SceneObject o;
		o.SetMesh(sceneMeshes[object["mesh"]]);
		o.SetMaterial(sceneMaterials[object["material"]]);
		o.SetPosition(Vector3(object["position"][0], object["position"][1], object["position"][2]));
		o.SetRotation(Vector3(object["rotation"][0], object["rotation"][1], object["rotation"][2]));
		o.SetScale(Vector3(object["scale"][0], object["scale"][1], object["scale"][2]));

		sceneObjects.push_back(o);
	}

	// count globalpolycount
	for (SceneObject& obj : sceneObjects) {
		globalPolyCount += (obj.GetMesh()).GetFaceCount();
	}

	// Setup the camera
	Vector3 camPos = Vector3(data["camera"]["position"][0], data["camera"]["position"][1], data["camera"]["position"][2]);
	Vector3 camRot = Vector3(data["camera"]["rotation"][0], data["camera"]["rotation"][1], data["camera"]["rotation"][2]);
	float camFocalLength = data["camera"]["focalLength"];

	cam.Apos = camPos;
	cam.Bpos = camPos + Vector3( 1, 1, 0);
	cam.Cpos = camPos + Vector3(-1, 1, 0);
	
	cam.focalLength = camFocalLength;
	cam.focalPoint = camPos + Vector3(0, 0, -camFocalLength);
	cam.lookAt =     camPos + Vector3(0, 0,  camFocalLength);

	rotateCamera(camRot);

	// Set the scene size
	if (data["sceneSize"] == "small") {
		size = KS_SCENESIZE_SMALL;
	}
	else if (data["sceneSize"] == "medium") {
		size = KS_SCENESIZE_MEDIUM;
	}
	else if (data["sceneSize"] == "large") {
		size = KS_SCENESIZE_LARGE;
	}
	else {
		size = KS_SCENESIZE_MEDIUM;
	}
};

SceneInformation::~SceneInformation() {

}

DXVector3 SceneInformation::untransformFromCam(DXVector3 vect) {
	XMVECTOR invQuat = XMQuaternionInverse(cam.prevRotQuat);
	vect -= cam.Apos;
	vect = XMVector3Rotate(vect, invQuat);
	return vect;
}

SceneSize SceneInformation::getSceneSize() {
	return size;
}

int SceneInformation::getGlobalPolyCount() {
	return globalPolyCount;
}

tuple<Vector3, Vector3, Vector3> SceneInformation::getTribyGlobalIndex(int idx) {
	int currentSO = 0;
	int accTriCount = 0;
	int nextFaceAmount;
	Mesh objMesh;
	
	for (SceneObject& obj : sceneObjects) {
		objMesh = obj.GetMesh();

		nextFaceAmount = objMesh.GetFaceCount();

		// if the index is less than the accumulated tri count then we are in range
		if (idx < accTriCount + nextFaceAmount) {
			// get the face index vector at position idx - accTriCount
			Vector3 faceIdxVec = objMesh.GetIndex(idx - accTriCount);

			// using each element in the index vector construct tuple from GetFinalVtx()
			return make_tuple(
				obj.GetFinalVtx((int)faceIdxVec.x), // explicit cast to get rid of warning
				obj.GetFinalVtx((int)faceIdxVec.y),
				obj.GetFinalVtx((int)faceIdxVec.z)
			);

		}

		// otherwise we need to move on to the next object
		currentSO++;
		accTriCount += nextFaceAmount;
	}
	//throw std::out_of_range("Triangle index out of range");

	// this should never happen, but return 0 vectors so it doesn't crash
	return make_tuple(Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0));
}

void SceneInformation::setCameraPos(DXVector3 newPos) {
	Vector3 offset = newPos - cam.Apos;
	
	cam.Apos = newPos;
	cam.Bpos += offset;
	cam.Cpos += offset;
	
	cam.focalPoint += offset;
	cam.lookAt += offset;

	cam.viewMatrix = ComputeViewMatrix();
}

// keep in mind this is not cumulative and will overwrite the current rotation
void SceneInformation::rotateCamera(DXVector3 newRot) {
	XMVECTOR rotQuat = XMQuaternionRotationRollPitchYawFromVector(newRot);
	cam.prevRotQuat = rotQuat;
	
	// Construct new camera vectors based off of Apos
	Vector3 newBpos = (Vector3) XMVector3Rotate(Vector3{  1, 1, 0 }, rotQuat) + cam.Apos;
	Vector3 newCpos = (Vector3) XMVector3Rotate(Vector3{ -1, 1, 0 }, rotQuat) + cam.Apos;

	Vector3 newUp   = (Vector3) XMVector3Rotate(Vector3{  0, 1, 0 }, rotQuat);
	Vector3 newFocalPoint = (Vector3)XMVector3Rotate(Vector3{ 0, 0, -cam.focalLength }, rotQuat) + cam.Apos;
	Vector3 newLookAt     = (Vector3)XMVector3Rotate(Vector3{ 0, 0,  cam.focalLength }, rotQuat) + cam.Apos;

	cam.Bpos = newBpos;
	cam.Cpos = newCpos;
	cam.up = newUp;
	cam.focalPoint = newFocalPoint;
	cam.lookAt = newLookAt;

	// compute view matrix, projection matrix is only recomputed when windows is resized
	// or clipping planes are changed
	cam.viewMatrix = ComputeViewMatrix();
}

void SceneInformation::setCameraFocalLength(float newFocalLength) {
	cam.focalLength = newFocalLength;
	Vector3 newFocalPoint = (Vector3)XMVector3Rotate(Vector3{ 0, 0, -cam.focalLength }, cam.prevRotQuat) + cam.Apos;
	
	// TODO: would also need to update FOV

	cam.focalPoint = newFocalPoint;
}

float SceneInformation::getCameraFocalLength() {
	return cam.focalLength;
}

XMMATRIX SceneInformation::ComputeViewMatrix() {
	// Compute the view matrix using the camera's position, look-at point, and up vector
	//return XMMatrixLookAtRH(cam.Apos, cam.focalPoint, cam.up);
	return XMMatrixLookAtRH(cam.Apos, cam.lookAt, Vector3{0, 1, 0});
}

XMMATRIX SceneInformation::ComputeProjectionMatrix(float newWidth, float newHeight) {
	//return XMMatrixPerspectiveRH(1000.0f, 800.0f, 0.01f, 100.0f);
	return  XMMatrixPerspectiveFovRH((40.0f/360.0f)*XM_2PI, newWidth / newHeight, 0.01f, 100.0f);
	//return XMMatrixIdentity();
}

void SceneInformation::UpdateScreenSize(int newWidth, int newHeight) {
	cam.projectionMatrix = ComputeProjectionMatrix(newWidth, newHeight);

	// TODO: update any screen size dependent variables here
}

Camera SceneInformation::getCam() {
	return cam;
}

string SceneInformation::getSceneName()
{
	return sceneName;
}

string SceneInformation::getSceneDescription()
{
	return sceneDescription;
}

string SceneInformation::getScenePath()
{
	return scenePath;
}

map<string, Mesh> SceneInformation::getSceneMeshes()
{
	return sceneMeshes;
}

map<string, Material> SceneInformation::getSceneMaterials()
{
	return sceneMaterials;
}

vector<SceneObject>& SceneInformation::getSceneObjects()
{
	return sceneObjects;
}
