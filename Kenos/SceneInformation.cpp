#include "pch.h"

#include "SceneInformation.h"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;

// Default constructor
SceneInformation::SceneInformation() {
	globalPolyCount = 0;
};

SceneInformation::SceneInformation(string filePath) {
	// Load the scene file
	ifstream f(filePath);
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


		
		// Load mesh vertices and faces from json file
		ifstream meshFile(mesh);
		json meshData = json::parse(meshFile);

		// Load vertices
		const auto vertsXArr = meshData["verts"][0];
		const auto vertsYArr = meshData["verts"][1];
		const auto vertsZArr = meshData["verts"][2];
		vector<Vector3> verts;
		for (int i = 0; i < vertsXArr.size(); i++) {
			verts.push_back(Vector3(vertsXArr[i], vertsYArr[i], vertsZArr[i]));
		}

		// Load faces
		const auto face1Arr = meshData["faces"][0];
		const auto face2Arr = meshData["faces"][1];
		const auto face3Arr = meshData["faces"][2];
		vector<Vector3> faces;
		for (int i = 0; i < face1Arr.size(); i++) {
			faces.push_back(Vector3(face1Arr[i], face2Arr[i], face3Arr[i]));
		}

		//Add mesh to the list of meshes in the scene
		sceneMeshes[mesh] = Mesh{ verts, faces };
		
	}

	// Create scene objects
	for (auto& object : data["objects"]) {
		SceneObject o;
		o.SetMesh(&sceneMeshes[object["mesh"]]);
		o.SetMaterial(&sceneMaterials[object["material"]]);
		o.SetPosition(Vector3(object["position"][0], object["position"][1], object["position"][2]));
		o.SetRotation(Vector3(object["rotation"][0], object["rotation"][1], object["rotation"][2]));
		o.SetScale(Vector3(object["scale"][0], object["scale"][1], object["scale"][2]));

		sceneObjects.push_back(o);
	}

	
};

SceneInformation::~SceneInformation() {

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

vector<SceneObject> SceneInformation::getSceneObjects()
{
	return sceneObjects;
}
