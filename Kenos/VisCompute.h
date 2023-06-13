#pragma once

#include <map>
#include <vector>
#include <DirectXMath.h>
#include <SimpleMath.h>

// Init VisArray (array of hashmaps, each hashmap contains visibility info for a face)
std::vector<std::map<DirectX::SimpleMath::Vector3, int>> d_VisArray;

// Init shadow array (array of hashmaps, each hashmap contains shadow info for a face)
std::vector<std::map<int, std::vector<int>>> d_ShdwArray;

// reflect/specular is no longer needed because you can just do a-b to get a reflection
// vector, since all refelction are 180
