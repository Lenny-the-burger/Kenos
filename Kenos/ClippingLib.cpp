#include "pch.h"
#include <DirectXMath.h>
#include <SimpleMath.h>
#include <algorithm>
#include <memory>
#include <map>

#include "ClippingLib.h"
#include "CoreFuncsLib.h"

using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

/* Clip point p into triangle tri
* 1. Check if the point to clip is within the triangle, if it is then 
	just return the point.
* 2. Project the point onto all three edges
* 3. Check if the two closest projections (by angle) are on the edges, 
	if neither are, return the closest vertex.
* 4. Else divide the projections by weather they are on an edge, 
	/1 if it is on /0 (+infty) if it is not. Return the closest one.
*/
Vector2 ClipVector(Vector2 tri[], Vector2 p) {
	// This isnt used for now so ill implement it later if needed
	tri;
	p;
	return Vector2();
}

/* Clip triangle tri1 into triangle tri2 (union operation)
* 1. Check if any b verts are inside of a, if they are append to outlist
* 2. Check if any a verts are inside of b, if they are append to outlist
* 3. Calculate intersections of each edge on triangle a for each edge of triangle b
* 4. Check if each intersection is on an edge of both triangle a and b, if they are append to outlist
* 5. Order the outlist by their angle relative to the clipped area centre
*/
Vector2* ClipTri(Vector2 tri1[], Vector2 tri2[]) {
	// This is all done in screen space at the very end, not ideal and leads to 
	// unnecesary calculation of polygons that wouldve been culled earlier
	// but it works for now

	// initialize outlist to max possible length of culled polygon,
	// if the last few are not used then they are set to (0,0) and 
	// the length is set to the actual length
	Vector2* outlist = new Vector2[6];
	int outlistLength = 0;

	// 1. Check if any b verts are inside of a, if they are append to outlist
	for (int i = 0; i < 3; i++) {
		if (IsWithinTriangle2(tri1[0], tri1[1], tri1[2], tri2[i])) {
			outlist[outlistLength] = tri2[i];
			outlistLength++;
		}
	}

	// 2. Check if any a verts are inside of b, if they are append to outlist
	for (int i = 0; i < 3; i++) {
		if (IsWithinTriangle2(tri2[0], tri2[1], tri2[2], tri1[i])) {
			outlist[outlistLength] = tri1[i];
			outlistLength++;
		}
	}

	// 3. Calculate intersections of each edge on triangle a for each edge of triangle b
	// iniitalize intersection arrays:
	Vector2* intersectionsA = new Vector2[3];
	Vector2* intersectionsB = new Vector2[3];
	Vector2* intersectionsC = new Vector2[3];

	// calculate intersections:
	for (int i = 0; i < 3; i++) { // loop over every edge in tri 1
		intersectionsA[i] = IntersectLinears(tri1[0], tri1[1], tri2[i], tri2[(i % 3) + 1]);
		intersectionsB[i] = IntersectLinears(tri1[1], tri1[2], tri2[i], tri2[(i % 3) + 1]);
		intersectionsC[i] = IntersectLinears(tri1[2], tri1[0], tri2[i], tri2[(i % 3) + 1]);
	}

	// 4. Check if each intersection is on an edge of both triangle a and b, if they are append to outlist
	// initialize array to hold bools of if things are within segments or not
	// one for tri1 and one for tri2, each 3x3. Intersections have to be on both to be added.
	bool isIn1[3][3];
	bool isIn2[3][3];
	
	
	// calculate weather the intersections are within line segments of tri2
	for (int i = 0; i < 3; i++) {
		isIn1[0][i] = IsWithinLineSegment(tri2[i], tri2[(i % 3) + 1], intersectionsA[i]);
		isIn1[1][i] = IsWithinLineSegment(tri2[i], tri2[(i % 3) + 1], intersectionsB[i]);
		isIn1[2][i] = IsWithinLineSegment(tri2[i], tri2[(i % 3) + 1], intersectionsC[i]);
	}
	
	// calculate weather the intersections are within line segments of tri1
	for (int i = 0; i < 3; i++) {
		isIn2[0][i] = IsWithinLineSegment(tri1[0], tri1[1], intersectionsA[i]);
		isIn2[1][i] = IsWithinLineSegment(tri1[1], tri1[3], intersectionsB[i]);
		isIn2[2][i] = IsWithinLineSegment(tri1[2], tri1[0], intersectionsC[i]);
	}

	// apply AND to the two arrays to get the final result
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			// if both are true then add to outlist
			if (isIn1[i][j] && isIn2[i][j]) {
				outlist[outlistLength] = intersectionsA[i];
				outlistLength++;
			}
		}
	}

	// 5. Order the outlist by their angle relative to the clipped area centre
	
	// Compute mean of outlist
	Vector2 outMean = Vector2(0, 0);
	for (int i = 0; i < outlistLength; i++) {
		outMean += outlist[i];
	}
	outMean /= (float) outlistLength;

	// Compute angles of each point relative to the mean
	float outlistAngles[6];
	for (int i = 0; i < outlistLength; i++) {
		outlistAngles[i] = atan2(outlist[i].y - outMean.y, outlist[i].x - outMean.x);
	}

	// Order the outlist by the angles by creating a hashmap that goes angle:Vector2
	// and then just sort the keys
	map<float, Vector2> outlistMap;

	// populate the map
	for (int i = 0; i < outlistLength; i++) {
		outlistMap[outlistAngles[i]] = outlist[i];
	}
	
	// repopulate the outlist in order
	for (auto const& x : outlistMap) {
		outlist[outlistLength] = x.second;
	}
	
	// return ordered outlist
	return outlist;
}