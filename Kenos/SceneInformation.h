#pragma once

#include <string>

#include "Mesh.h"
#include "Material.h"
#include "SceneObject.h"

/*
* Contains all of the information for a scene. Usually used as a transition from scene.json to runtime
* and vise versa. This will eventually be used to save compute light tree and visibility data to disk.
*/

class SceneInformation
{
public:
	SceneInformation(std::string sceneName);
	~SceneInformation();

private:
	// Scene object arrays
	std::map<std::string, Mesh> sceneMeshes;
	std::map<std::string, Material> sceneMaterials;
	std::vector<SceneObject> sceneObjects;
	
	std::string sceneName;
	std::string sceneDescription;
	std::string scenePath;
};