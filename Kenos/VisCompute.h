#pragma once

#include <map>
#include <vector>
#include <DirectXMath.h>
#include <SimpleMath.h>

#include "SceneInformation.h"

struct FaceVisibilityData;

// Init VisArray (array of hashmaps, each hashmap contains visibility info for a face)
std::vector<std::map<DirectX::SimpleMath::Vector3, int>> d_VisArray;

// reflect/specular is no longer needed because you can just do a-b to get a reflection
// vector, since all refelction are 180
