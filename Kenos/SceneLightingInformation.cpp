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
	finalDirectoryBuffer.clear();

	// for now just loop through all of the polygons and assembly the directory buffer with the
	// albedo material colour for each object

	for (int i = 0; i < globalPolyCount; i++) {
		// get the object that this poly belongs to
		int objIdx = scene.getObjIndexbyGlobalIndex(i);

		// get the object
		SceneObject obj = scene.getSceneObjects()[objIdx];

		// get the material
		Material mat = obj.GetMaterial();

		// get the albedo colour
		Color albedo = mat.GetAlbedo();

		SurfaceLightmapDirectoryPacked dir;
		dir.color = XMFLOAT3(albedo.x, albedo.y, albedo.z)/255.0f; // convert to 0-1 range

		dir.emmissiveStrength = mat.GetEmissiveIntensity();

		finalDirectoryBuffer.push_back(dir);
	}
}

std::vector<SurfaceLightmapDirectoryPacked> SceneLightingInformation::GetDirectoryBuffer() {
	return finalDirectoryBuffer;
}

std::vector<SurfLight> SceneLightingInformation::GetFinalLightmapBuffer() {
	return finalLightmapBuffer;
}