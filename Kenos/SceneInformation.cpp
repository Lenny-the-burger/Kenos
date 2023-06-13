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

};

SceneInformation::SceneInformation(std::string filePath) {
	// Load the scene file
	std::ifstream f(filePath);
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
			std::string errorMessage = "Mesh file '" + mesh + "' does not exist";
			throw std::exception(errorMessage.c_str());
		}
		// TODO: load file here
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

};