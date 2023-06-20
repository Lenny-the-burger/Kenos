#include "pch.h"
#include "SceneLightingInformation.h"

using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;


SceneLightingInformation::SceneLightingInformation()
{
	// initialize memebr vars so vs dont complain
	scene = nullptr;
	globalPolyCount = 0;
}

SceneLightingInformation::~SceneLightingInformation()
{
}

void SceneLightingInformation::SetScene(SceneInformation* newScene)
{
	scene = newScene;

	// recalculate the global poly count
	vector<SceneObject> sceneObjects = scene->getSceneObjects();

	// count globalpolycount
	for (SceneObject& obj : sceneObjects) {
		globalPolyCount += (obj.GetMesh()).GetFaceCount();
	}
}

void SceneLightingInformation::BuildLightTree() {

}

void SceneLightingInformation::UpdateLightTree(int idx) {
	idx;

}

void SceneLightingInformation::UpdateFinalRDFBuffer() {

}

vector<ScreeSpaceRDF> SceneLightingInformation::GetFinalRDFBuffer() {
	return finalRDFbuffer;
}