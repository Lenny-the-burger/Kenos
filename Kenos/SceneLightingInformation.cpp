#include "pch.h"
#include "SceneLightingInformation.h"

using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;

SceneLightingInformation::SceneLightingInformation() :
	globalPolyCount(0),
	scene(*new SceneInformation())
{
	// initialize memebr vars so vs dont complain
}

SceneLightingInformation::SceneLightingInformation(SceneInformation& newScene) : 
	scene(newScene),
	globalPolyCount(0)
{
	// initialize memebr vars so vs dont complain
}

SceneLightingInformation::~SceneLightingInformation()
{
}

void SceneLightingInformation::SetScene(SceneInformation& newScene)
{
	scene = newScene;

	// recalculate the global poly count
	vector<SceneObject> sceneObjects = scene.getSceneObjects();

	// count globalpolycount
	for (SceneObject& obj : sceneObjects) {
		globalPolyCount += (obj.GetMesh()).GetFaceCount();
	}
}

void SceneLightingInformation::SetScreenRatio(float ratio)
{
	screenRatio = ratio;
}

void SceneLightingInformation::BuildLightTree() {
	// for now we will use stdev = 125 * dist for the FRDF
	// probably wrong but seems correct so lol

}

void SceneLightingInformation::UpdateLightTree(int idx) {
	idx;

}

void SceneLightingInformation::UpdateFinalRDFBuffer() {
	finalRDFbuffer.clear();
	
	Camera localCamera = scene.getCam();

	XMVECTOR camPlane = XMPlaneFromPoints(localCamera.Apos, localCamera.Bpos, localCamera.Cpos);

	tuple<Vector3, Vector3, Vector3> currTri;

	Vector3 halfScreenVect = Vector3(1920 / 2, 1080 / 2, 0);

	Vector3 screenRatioVector = Vector3(1/screenRatio, 1, 0);

	for (int globalIndex = 0; globalIndex < scene.getGlobalPolyCount(); globalIndex++) {
		currTri = scene.getTribyGlobalIndex(globalIndex);

		Vector3 v1 = get<0>(currTri);
		Vector3 v2 = get<1>(currTri);
		Vector3 v3 = get<2>(currTri);

		// do backface culling
		Vector3 normal = (v2 - v1).Cross(v3 - v1);
		if (normal.Dot(localCamera.focalPoint - (v1 + v2 + v3) / 3) < 0) {
			continue;
		}

		// intersect the camera plane with a line that goes through each vert
		// of the triangle and the focal point
		v1 = XMPlaneIntersectLine(camPlane, localCamera.focalPoint, v1);
		v2 = XMPlaneIntersectLine(camPlane, localCamera.focalPoint, v2);
		v3 = XMPlaneIntersectLine(camPlane, localCamera.focalPoint, v3);

		// normalize points
		v1 = scene.untransformFromCam(v1);
		v2 = scene.untransformFromCam(v2);
		v3 = scene.untransformFromCam(v3);

		/*v1 *= 100;
		v2 *= 100;
		v3 *= 100;*/

		// assuming a 1920x1080 screen resolution for now
		/*v1 += halfScreenVect;
		v2 += halfScreenVect;
		v3 += halfScreenVect;*/

		// flip the y axis because the screen is upside down for some reason
		/*v1.y = 1080 - v1.y;
		v2.y = 1080 - v2.y;
		v3.y = 1080 - v3.y;*/

		// scale it down a bit
		v1 /= 5;
		v2 /= 5;
		v3 /= 5;

		// scale it to fit the screen
		v1 *= screenRatioVector;
		v2 *= screenRatioVector;
		v3 *= screenRatioVector;

		// Color based on global index
		float idxRatio = (float)globalIndex / (float)scene.getGlobalPolyCount();
		Color color = Color{ idxRatio, idxRatio, idxRatio };

		// Construct the SSRD from the RDF and add into the final buffer
		tuple<Vector3, Vector3, Vector3> ssrdf_bounds = { v1, v2, v3 };
		
		ScreeSpaceRDF ssrdf = ScreeSpaceRDF{ ssrdf_bounds, color };

		finalRDFbuffer.push_back(ssrdf);

		// TODO: extract the stdev vector from the RDF and properly do other tsuff
		
	}
}

vector<ScreeSpaceRDF> SceneLightingInformation::GetFinalRDFBuffer() {
	return finalRDFbuffer;
}