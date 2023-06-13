#pragma once

#ifndef RENDER_MATH_H
#define RENDER_MATH_H

/* 
	This file contains the core math functions used by the engine that arent covered by DirectXMath.
*/

#include <memory>
#include <DirectXMath.h>
#include <vector>

using DXVector3 = DirectX::SimpleMath::Vector3;

// Creates a function that goes through points a, b, and returns f(x)
// 
// Params:
//		a: The first point (2d vector)
//		b: The second point (2d vector)
//		x: The x value to get the y value of (scalar)
// Returns:
//		The y value of the function at x (scalar)
float LinearFuncAB(DirectX::SimpleMath::Vector2 a, DirectX::SimpleMath::Vector2 b, float x);

// Returns is point c is within the line segment ab (assumes that all are colinear)
//
// Params:
//		a: The first point (2d vector)
//		b: The second point (2d vector)
//		c: The point to check (2d vector)
// Returns:
//		True if c is within the line segment ab, false otherwise (bool)
bool IsWithinLineSegment(DirectX::SimpleMath::Vector2 a, DirectX::SimpleMath::Vector2 b, DirectX::SimpleMath::Vector2 c);

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
DirectX::SimpleMath::Vector2 IntersectLinears(DirectX::SimpleMath::Vector2 a1, DirectX::SimpleMath::Vector2 a2, DirectX::SimpleMath::Vector2 b1, DirectX::SimpleMath::Vector2 b2);

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
DirectX::SimpleMath::Vector2 ProjectPointOntoLine(DirectX::SimpleMath::Vector2 a, DirectX::SimpleMath::Vector2 b, DirectX::SimpleMath::Vector2 p);

// Attenuates a value based on distance from a point
//
// Params:
//		dist: The distance from the point (scalar)
//		value: The value to attenuate (scalar)
// Returns:
//		The attenuated value (scalar)
float Attenuate(float dist, float value);

// Convert 3D cartesian into baryocentric coordinates
//
// Params:
//		a: The first point of the triangle
//		b: The second point of the triangle
//		c: The third point of the triangle
//		p: The point to convert
// Returns:
//		The baryocentric coordinates of the point (3d vector, u, v, w)
DirectX::SimpleMath::Vector3 CartesianToBaryocentric3(DirectX::SimpleMath::Vector3 a, DirectX::SimpleMath::Vector3 b, DirectX::SimpleMath::Vector3 c, DirectX::SimpleMath::Vector3 p);
DirectX::SimpleMath::Vector3 CartesianToBaryocentric2(DirectX::SimpleMath::Vector2 a, DirectX::SimpleMath::Vector2 b, DirectX::SimpleMath::Vector2 c, DirectX::SimpleMath::Vector2 p);

// Test if point p is within triangle abc
//
// Params:
//		a: The first point of the triangle
//		b: The second point of the triangle
//		c: The third point of the triangle
//		p: The point to test
// Returns:
//		True if p is within triangle abc, false otherwise (bool)
bool IsWithinTriangle3(DirectX::SimpleMath::Vector3 a, DirectX::SimpleMath::Vector3 b, DirectX::SimpleMath::Vector3 c, DirectX::SimpleMath::Vector3 p);
bool IsWithinTriangle2(DirectX::SimpleMath::Vector2 a, DirectX::SimpleMath::Vector2 b, DirectX::SimpleMath::Vector2 c, DirectX::SimpleMath::Vector2 p);

// copmute normal of triangle abc
DirectX::SimpleMath::Vector3 GetNormal(DirectX::SimpleMath::Vector3 a, DirectX::SimpleMath::Vector3 b, DirectX::SimpleMath::Vector3 c);

// project point p onto plane defined by abc
DirectX::SimpleMath::Vector3 ProjectABC(DirectX::SimpleMath::Vector3 a, DirectX::SimpleMath::Vector3 b, DirectX::SimpleMath::Vector3 c, DirectX::SimpleMath::Vector3 p);

#endif