#pragma once

#include "pch.h"
#include <DirectXMath.h>
#include <SimpleMath.h>

/*
* Kenos clipping library, clip vectors and triangles into triangle.
*/

// Function that clips single vector (p) into triangle defined by a (assumed coplanar)
DirectX::SimpleMath::Vector2 ClipVector(DirectX::SimpleMath::Vector2 tri[], DirectX::SimpleMath::Vector2 p);

// Function that clips a triangle into another triangle (union) (assumed coplanar)
DirectX::SimpleMath::Vector2* ClipTri(DirectX::SimpleMath::Vector2 tri1[], DirectX::SimpleMath::Vector2 tri2[]);
