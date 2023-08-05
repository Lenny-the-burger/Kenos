/*
	This file contains the core math functions used by the engine that arent covered by DirectXMath.
*/

#include "pch.h"
#include <memory>
#include <vector>
#include <algorithm>

#include "CoreFuncsLib.h"
#include <DirectXMath.h>
#include <SimpleMath.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

#include <DirectXMath.h>

#include <DirectXMath.h>

DirectX::XMMATRIX OrthographicProjectionOntoPlane(const DirectX::XMVECTOR plane)
{
	using namespace DirectX;

	// Extract the plane coefficients
	float A = XMVectorGetX(plane);
	float B = XMVectorGetY(plane);
	float C = XMVectorGetZ(plane);
	float D = XMVectorGetW(plane);

	// Compute the normalization factor
	float norm = sqrtf(A * A + B * B + C * C);

	// Normalize the plane coefficients
	A /= norm;
	B /= norm;
	C /= norm;
	D /= norm;

	// Construct the orthographic projection matrix
	XMMATRIX projectionMatrix =
	{
		1.0f - A * A,   -A * B,       -A * C,       0.0f,
		-A * B,        1.0f - B * B, -B * C,       0.0f,
		-A * C,        -B * C,       1.0f - C * C, 0.0f,
		-A * D,        -B * D,       -C * D,       1.0f
	};

	return projectionMatrix;
}



// Creates a function that goes through points a, b, and returns f(x)
// 
// Params:
//		a: The first point (2d vector)
//		b: The second point (2d vector)
//		x: The x value to get the y value of (scalar)
// Returns:
//		The y value of the function at x (scalar)
float LinearFuncAB(Vector2 a, Vector2 b, float x) {
	float m = (a.y - b.y) / (a.x - b.x);
	return m * (x - a.x) + a.y;
};

// Returns is point c is within the line segment ab (assumes that all are colinear)
//
// Params:
//		a: The first point (2d vector)
//		b: The second point (2d vector)
//		c: The point to check (2d vector)
// Returns:
//		True if c is within the line segment ab, false otherwise (bool)
bool IsWithinLineSegment(Vector2 a, Vector2 b, Vector2 c) {
	float abdot = (a - c).Dot(b - c);
	
	return abdot < 0;
};

// Returns the intersection of two lines defined by points a1, a2, b1, b2
// It does this algabraicly so the implementation is a bit messy
//
// Params:
//		a1: The first point of the first line (2d vector)
//		a2: The second point of the first line (2d vector)
//		b1: The first point of the second line (2d vector)
//		b2: The second point of the second line (2d vector)
// Returns:
//		The intersection of the two lines (2d vector)
Vector2 IntersectLinears(Vector2 a1, Vector2 a2, Vector2 b1, Vector2 b2) {
	float interX = ((a1.x * a2.y - a1.y * a2.x) * (b1.x - b2.x) - (a1.x - a2.x) * (b1.x * b2.y - b1.y * b2.x)) / ((a1.x - a2.x) * (b1.y - b2.y) - (a1.y - a2.y) * (b1.x - b2.x));
	float interY = ((a1.x * a2.y - a1.y * a2.x) * (b1.y - b2.y) - (a1.y - a2.y) * (b1.x * b2.y - b1.y * b2.x)) / ((a1.x - a2.x) * (b1.y - b2.y) - (a1.y - a2.y) * (b1.x - b2.x));
	return Vector2(interX, interY);
};

// Returns the position of the smallest/largest object in an array
//
// Params:
//		arr: The array to search
// Returns:
//		The position of the smallest/largest object in the array (int)
int PosMin(float arr[]);
int PosMax(float arr[]);

// Remove an element from an array and shift the rest of the elements down
//
// Params:
//		arr: The array to remove from
//		pos: The position of the element to remove
// Returns:
//		The new array (float*)
std::unique_ptr<float[]> RemoveElement(float arr[], int pos);

// Project a 2d vector onto a line defined by two other vectors
//
// Params:
//		a: The first point of the line (2d vector)
//		b: The second point of the line (2d vector)
//		p: The point to project (2d vector)
// Returns:
//		The projected point (2d vector)
Vector2 ProjectPointOntoLine(Vector2 a, Vector2 b, Vector2 p) {
	Vector2 bProj = a - b;
	Vector2 cProj = bProj * (p.Dot(bProj) / bProj.Dot(bProj));
	// yes, dot(b, b) is not a typo, blame the person that helped get me proection working for it.
	Vector2 dProj = a + (a.Dot(bProj) / bProj.Dot(bProj)) * (b - a);
	
	return dProj + cProj;
};

// Attenuates a value based on distance from a point
//
// Params:
//		dist: The distance from the point (scalar)
//		value: The value to attenuate (scalar)
// Returns:
//		The attenuated value (scalar)
float Attenuate(float dist, float value) {
	return value / (dist * dist);
};

// Convert 3D cartesian into baryocentric coordinates
//
// Params:
//		a: The first point of the triangle
//		b: The second point of the triangle
//		c: The third point of the triangle
//		p: The point to convert
// Returns:
//		The baryocentric coordinates of the point (3d vector, u, v, w)
Vector3 CartesianToBaryocentric3(Vector3 a, Vector3 b, Vector3 c, Vector3 p) {
	Vector3 v0 = b - a, v1 = c - a, v2 = p - a;
	float d00 = v0.Dot(v0);
	float d01 = v0.Dot(v1);
	float d11 = v1.Dot(v1);
	float d20 = v2.Dot(v0);
	float d21 = v2.Dot(v1);
	float denom = d00 * d11 - d01 * d01;
	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1.0f - v - w;
	return Vector3(u, v, w);
};
Vector3 CartesianToBaryocentric2(Vector2 a, Vector2 b, Vector2 c, Vector2 p) {
	Vector2 v0 = b - a, v1 = c - a, v2 = p - a;
	float d00 = v0.Dot(v0);
	float d01 = v0.Dot(v1);
	float d11 = v1.Dot(v1);
	float d20 = v2.Dot(v0);
	float d21 = v2.Dot(v1);
	float denom = d00 * d11 - d01 * d01;
	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1.0f - v - w;
	return Vector3(u, v, w);
};

// Test if point p is within triangle abc
//
// Params:
//		a: The first point of the triangle
//		b: The second point of the triangle
//		c: The third point of the triangle
//		p: The point to test
// Returns:
//		True if p is within triangle abc, false otherwise (bool)
bool IsWithinTriangle3(Vector3 a, Vector3 b, Vector3 c, Vector3 p) {
	Vector3 bary = CartesianToBaryocentric3(a, b, c, p);
	return bary.x >= 0 && bary.y >= 0 && bary.z >= 0;
};

bool IsWithinTriangle2(Vector2 a, Vector2 b, Vector2 c, Vector2 p) {
	Vector3 bary = CartesianToBaryocentric2(a, b, c, p);
	return bary.x >= 0 && bary.y >= 0 && bary.z >= 0;
}

Vector3 GetNormal(Vector3 a, Vector3 b, Vector3 c)
{
	Vector3 temp = (((a - b).Cross((a - c))));
	return XMVector3Normalize(temp);
}

// project point p onto plane defined by abc
Vector3 ProjectABC(Vector3 a, Vector3 b, Vector3 c, Vector3 p)
{
	XMVECTOR planeCoefficients = XMPlaneFromPoints(a, b, c);
	Vector3 result = XMPlaneIntersectLine(planeCoefficients, GetNormal(a, b, c), p);
	
	return result;
}
;