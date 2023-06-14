#pragma once

#include <map>
#include <vector>
#include <DirectXMath.h>
#include <SimpleMath.h>

#include "SceneInformation.h"

struct FaceVisibilityData;

// Init VisArray (array of hashmaps, each hashmap contains visibility info for a face)
std::vector<std::map<DirectX::SimpleMath::Vector3, int>> d_VisArray;

vector<SceneObject> sceneObjects;

// return triangle at global index idx
std::tuple<DirectX::SimpleMath::Vector3, DirectX::SimpleMath::Vector3, DirectX::SimpleMath::Vector3> getTribyGlobalIndex(int idx);