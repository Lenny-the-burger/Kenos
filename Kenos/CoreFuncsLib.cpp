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
#include <math.h> 

using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

bool BVHintesects(const Ray& ray, const Vector3& Vmin, const Vector3& Vmax) {
	float tmin, tmax, tymin, tymax, tzmin, tzmax;

	tmin = (Vmin.x - ray.position.x) / ray.direction.x;
	tmax = (Vmax.x - ray.position.x) / ray.direction.x;

	if (tmin > tmax)
		std::swap(tmin, tmax);

	tymin = (Vmin.y - ray.position.y) / ray.direction.y;
	tymax = (Vmax.y - ray.position.y) / ray.direction.y;

	if (tymin > tymax)
		std::swap(tymin, tymax);

	if ((tmin > tymax) || (tymin > tmax))
		return false;

	if (tymin > tmin)
		tmin = tymin;

	if (tymax < tmax)
		tmax = tymax;

	tzmin = (Vmin.z - ray.position.z) / ray.direction.z;
	tzmax = (Vmax.z - ray.position.z) / ray.direction.z;

	if (tzmin > tzmax)
		std::swap(tzmin, tzmax);

	if ((tmin > tzmax) || (tzmin > tmax))
		return false;

	return true;
}

// Function to create the transformation matrix from 3D to 2D.
// !! keep in mind that the updirection is normalized, it does this for you but 
// you may get unexpected results if you pass in a non-normalized vector and forget about this !!
XMMATRIX CreateTransformTo2D(XMVECTOR planeCoefficients, Vector3 upDirection) {
	// Extract the normal vector from the plane coefficients
	XMVECTOR normal = XMVector3Normalize(XMVectorSetW(planeCoefficients, 0.0f));

	// Create the up vector
	XMVECTOR up = XMLoadFloat3(&upDirection);
	up = XMVector3Normalize(up);

	// Compute the right (x-axis) vector within the plane
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(normal, up));

	// Compute the translation components based on the dot product with the plane coefficients
	float translationRight = -XMVectorGetX(XMVector3Dot(right, planeCoefficients));
	float translationUp = -XMVectorGetX(XMVector3Dot(up, planeCoefficients));
	float translationNormal = -XMVectorGetX(XMVector3Dot(normal, planeCoefficients));

	// Create the transformation matrix using the right, up, and normal vectors, along with the translation components
	XMMATRIX transformMatrix = {
		right.m128_f32[0], up.m128_f32[0], normal.m128_f32[0], translationRight,
		right.m128_f32[1], up.m128_f32[1], normal.m128_f32[1], translationUp,
		right.m128_f32[2], up.m128_f32[2], normal.m128_f32[2], translationNormal,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	return transformMatrix;
}

// Function to create the transformation matrix from 2D to 3D.
XMMATRIX CreateTransformTo3D(XMVECTOR planeCoefficients, Vector3 upDirection)
{
	// Create the 3D to 2D transformation matrix.
	XMMATRIX transformTo2D = CreateTransformTo2D(planeCoefficients, upDirection);

	// Return the inverse of the transformation matrix to get the 2D to 3D transformation.
	return XMMatrixInverse(nullptr, transformTo2D);
}

float normalDist(float mean, float stdev, float x) {
	return (1 / (stdev * 2.50662827463f)) * exp(-0.5f * pow((x - mean) / stdev, 2));
}

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