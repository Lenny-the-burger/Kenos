#pragma once

#include <map>
#include <vector>
#include <DirectXMath.h>
#include <SimpleMath.h>

#include "CoreFuncsLib.h"

#include "Mesh.h"
#include "Material.h"
#include "SceneObject.h"

#include "SceneInformation.h"

// Struct for storing visibility data, holds shadows, mean path distance
struct FaceVisibilityData {
	// Index of reciever
	int receiverIdx;

	// Shortest distance between caster and reciever polygon (the mean path)
	float meanPathDist;

	// dot target (solid angle) used for dot target culling
	float dotTarget;
};

// Init VisArray (array of hashmaps, each hashmap contains visibility info for a face)
std::vector<std::map<DirectX::SimpleMath::Vector3, FaceVisibilityData>> d_VisArray;

std::vector<SceneObject> sceneObjects;

int globalPolyCount;

// return triangle at global index idx
std::tuple<DirectX::SimpleMath::Vector3, DirectX::SimpleMath::Vector3, DirectX::SimpleMath::Vector3> getTribyGlobalIndex(int idx);

// recopmute the entire visibilitu array from scratch
void recomputeVisArray(SceneInformation scene);