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
	// for now we will use stdev = 10 * dist for the FRDF

	// loop through every triangle and create a lightmap directory for it
	for (int i = 0; i < globalPolyCount; i++) {
		// get the object that this poly belongs to
		int objIdx = scene.getObjIndexbyGlobalIndex(i);

		// get the object
		SceneObject obj = scene.getSceneObjects()[objIdx];

		// get the material
		Material mat = obj.GetMaterial();

		SurfaceLightmapDirectory dir = {};
		dir.color = mat.GetAlbedo();

		tuple<Vector3, Vector3, Vector3> verts = scene.getTribyGlobalIndex(i);

		XMVECTOR surfPLane = XMPlaneFromPoints(get<0>(verts), get<1>(verts), get<2>(verts));
		dir.flattenMatrix = OrthographicProjectionOntoPlane(surfPLane);
		dir.toPlaneLocalMatrix = CreateTransformTo2D(surfPLane, get<0>(verts) - get<1>(verts));
		dir.toWorldMatrix = CreateTransformTo3D(surfPLane, get<0>(verts) - get<1>(verts));

		// For now empty
		dir.surfLights = vector<int>();

		lightmapDirectories.push_back(dir);
	}

	// Compute normals for each triangle
	for (int i = 0; i < globalPolyCount; i++) {
		tuple<Vector3, Vector3, Vector3> currTri = scene.getTribyGlobalIndex(i);
		Vector3 triNormal = XMVector3Normalize(XMVector3Cross(get<1>(currTri) - get<0>(currTri), get<2>(currTri) - get<0>(currTri)));
		allNormals.push_back(triNormal);
	}

	// This is just to see how many surfaces are visible on average
	float avgVisSurfs = 0.0f;

	// Go through all of the directories and determine visibility structure
	// r_* is reciever, c_* is caster. Iterate over every surface (caster) and determine if it scatters
	// onto what surfaces (reciever)
	for (int dirIdx = 0; dirIdx < globalPolyCount; dirIdx++) {
		SurfaceLightmapDirectory currentDir = lightmapDirectories[dirIdx];

		tuple<Vector3, Vector3, Vector3> c_tri = scene.getTribyGlobalIndex(dirIdx);
		Vector3 c_triNormal = allNormals[dirIdx];
		Vector3 c_triMean = (get<0>(c_tri) + get<1>(c_tri) + get<2>(c_tri)) / 3.0f;


		// Use scene object BV to determine which objects are visible
		// (https://www.desmos.com/geometry-beta/twesb3a3o8)
		vector<int> visibleObjects = {};
		for (int i = 0; i < scene.getSceneObjects().size(); i++) {
			SceneObject obj = scene.getSceneObjects()[i];

			// If both the min and max are behind the current surface, then the object is not visible
			Vector3 objMin = get<0>(obj.getBVH());
			Vector3 objMax = get<1>(obj.getBVH());

			vector<Vector3> objCorners = { objMin,
							Vector3(objMin.x, objMin.y, objMax.z),
							Vector3(objMin.x, objMax.y, objMin.z),
							Vector3(objMin.x, objMax.y, objMax.z),
							Vector3(objMax.x, objMin.y, objMin.z),
							Vector3(objMax.x, objMin.y, objMax.z),
							Vector3(objMax.x, objMax.y, objMin.z),
							objMax };

			for (Vector3 corner : objCorners) {
				if (c_triNormal.Dot(corner - c_triMean) < 0.0f) {
					visibleObjects.push_back(i);
					break;
				}
			}

			// Object is invisible
		}

		currentDir.visibleObjects = visibleObjects;

		vector<int> visibleSurfaces = {};
		Vector3 r_Normal;
		// Reciever triangle
		Vector3 r_tri[3];

		// Assemble all the visible surfaces
		for (int i : visibleObjects) {
			int objRangeStart = scene.getObjectTrisRange(i).first;
			int objRangeEnd = scene.getObjectTrisRange(i).second;

			for (int j = objRangeStart; j < objRangeEnd; j++) {
				// Do visibility check per triangle
				scene.getTribyGlobalIndexFast(r_tri, j);

				// TODO: why does this particular line take so much cpu time? Im acessing a 
				// c style array?? VS profiler says it takes up ~70% of the cpu time in this function???
				r_Normal = allNormals[j]; 

				// We cant do regular backface culling because we are using an area light model
				// just becasue the normal is facing away from the camera doesnt mean it isnt visible
				// from another part of the surface. Loop over every vertex and see if it is visible
				// from the current surface. If all of them are not then the surface is not visible
				bool r_isVis[3];
				for (int vertIdx = 0; vertIdx < 3; vertIdx++) {
					Vector3 r_triVertex = r_tri[vertIdx];

					r_isVis[vertIdx] = c_triNormal.Dot(r_triVertex - c_triMean) > 0.0f;
				}
				
				// if all of the vertices are not visible hen the surface is not visible
				if (r_isVis[0] && r_isVis[1] && r_isVis[2]) {
					continue;
				}
				

				// If the triangle is behind the current surface, it is not visible
				for (int vertIdx = 0; vertIdx < 3; vertIdx++) {
					Vector3 r_triVertex = r_tri[vertIdx];

					// if the vertex is in front of the surface, then the surface is visible
					if (c_triNormal.Dot(r_triVertex - c_triMean) < 0.0f) {
						visibleSurfaces.push_back(j);
						break;
					}
				}

				// Surface is invisible
			}
		}

		currentDir.visibleSurfaces = visibleSurfaces;
		lightmapDirectories[dirIdx] = currentDir;

		avgVisSurfs += visibleSurfaces.size();
	}

	// Avg number of surfaces visible from each surface
	avgVisSurfs /= globalPolyCount;

	// What % of surfaces are visible from each surface
	avgVisSurfs /= globalPolyCount;

	// We are now 1/avgVisSurfs times faster than the naive approach (usually 10x faster)

	// loop through all the triangles and figure out which ones are emissive (emissive strength > 0.1)
	// and add them to the emissivePolygons vector
	for (int i = 0; i < globalPolyCount; i++) {
		// Check what sceneobject this poly belongs to
		int objIdx = scene.getObjIndexbyGlobalIndex(i);

		// get the object
		SceneObject obj = scene.getSceneObjects()[objIdx];

		// get the material
		Material mat = obj.GetMaterial();

		if (mat.GetEmissiveIntensity() > 0.1f) {
			emissivePolygons.push_back(i);
		}
	}

	// Loop over every emissive polygon and create a RDF for it and put it in the light tree roots
	for (int i: emissivePolygons) {
		// get the object that this poly belongs to
		int objIdx = scene.getObjIndexbyGlobalIndex(i);

		// get the object
		SceneObject obj = scene.getSceneObjects()[objIdx];

		// get the material
		Material mat = obj.GetMaterial();

		// create a RDF for this emissive polygon
		RDF rdf = {};

		// this is equivalent to the global index of the polygon
		rdf.parentDirectoryIndex = i;
		rdf.bounce = 0;
		rdf.parentRDF = -1;
		// children will be created later in the main loop
		rdf.color = mat.GetAlbedo();
		rdf.lightBrightness = mat.GetEmissiveIntensity();
		// At the source lightness is equivalent to the initial brightness
		rdf.lightness = mat.GetEmissiveIntensity();
		// A Light source cannot shadow itself, empty vector
		rdf.shadows = vector<int>();

		SurfaceLightmapDirectory parentDir = lightmapDirectories[i];

		parentDir.surfLights.push_back((int) jumbleMap.size());
		lightTree.push_back((int) jumbleMap.size());

		lightmapDirectories[i] = parentDir;
		jumbleMap.push_back(rdf);
	}

	// add all of the light tree roots to the process queue
	for (int i : lightTree) {
		processQueue.push(i);
	}
	
	// every surface scatters onto every other surface times number of bounces
	int maxIterations = globalPolyCount * globalPolyCount * KS_MAX_RAY_BOUNCES + 100;
	int iter = 0;

	// so we dont have to keep calling this
	int sceneObjectsSize = scene.getSceneObjects().size();

	// The main loop (this is where ray tracing happens)
	while (!processQueue.empty()) {
		// c_* for caster, r_* for receiver, s_* for shadow

		int c_RDFidx = processQueue.front();
		processQueue.pop();

		// get all the stuff
		RDF c_RDF = jumbleMap[c_RDFidx];
		int c_globalIdx = c_RDF.parentDirectoryIndex;
		SurfaceLightmapDirectory c_Dir = lightmapDirectories[c_globalIdx];

		Vector3 c_tri[3];
		scene.getTribyGlobalIndexFast(c_tri, c_globalIdx);
		Vector3 c_triMean = (c_tri[0] + c_tri[1] + c_tri[2]) / 3.0f;
		Vector3 c_Normal = allNormals[c_globalIdx];
		
		vector<int> c_visibleSurfaces = c_Dir.visibleSurfaces;

		Vector3 r_tri[3];

		int c_bounce = c_RDF.bounce;

		// Loop over every visible surface and ray trace
		for (int r_childIdx : c_visibleSurfaces) {
			SurfaceLightmapDirectory r_Dir = lightmapDirectories[r_childIdx];

			scene.getTribyGlobalIndexFast(r_tri, r_childIdx);
			Vector3 r_triMean = (r_tri[0] + r_tri[1] + r_tri[2]) / 3.0f;
			Vector3 r_Normal = allNormals[r_childIdx];

			// construct the reciever rdf
			RDF r_RDF = {};
			r_RDF.parentDirectoryIndex = r_childIdx;
			r_RDF.bounce = c_bounce + 1;
			r_RDF.parentRDF = c_RDFidx; // !! index into jumbleMap, not global index !!
			// children are assigned when the parent is processed
			// ignore colour for now for testing
			r_RDF.lightBrightness = c_RDF.lightBrightness;
			r_RDF.lightness = 1.0f; // going to have to compute lightness here
			
			// for now just ignore shadows, it is computed naivly in the pixel shader.
			// we use dot target culling so it should be fast enough for now

			// assuming that this surface passes lightness cull
			jumbleMap.push_back(r_RDF);
			int r_RDFidx = (int) jumbleMap.size() - 1;
			c_RDF.children.push_back(r_RDFidx);

			r_Dir.surfLights.push_back(r_RDFidx);
			lightmapDirectories[r_childIdx] = r_Dir;
		}
		
		// We will then spawn children processes for each of the visible surfaces
		// (minus lightness exclusions) and add them to the process queue.
		// The actual ray tracing happens when we flatten the lightmap
		// (for now in UpdateFinalRDFBuffer)

		iter++;
		if (iter > maxIterations) {
			// we have exceeded the maximum number of iterations oh no
			break;
		}
	}
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

		dir.normal = XMFLOAT3(allNormals[i].x, allNormals[i].y, allNormals[i].z);

		dir.emmissiveStrength = mat.GetEmissiveIntensity();

		dir.numLights = min((int)lightmapDirectories[i].surfLights.size(), KS_MAX_SURFACE_LIGHTS);

		// store the flatten matrix, to 2d and back to 3d matrices
		// The constant buffer matrices are transposed so this also should be transposed???
		// cos hlsl
		XMStoreFloat4x4(&dir.flattenMatrix, XMMatrixTranspose(lightmapDirectories[i].flattenMatrix));
		XMStoreFloat4x4(&dir.toPlaneLocalMatrix, XMMatrixTranspose(lightmapDirectories[i].toPlaneLocalMatrix));
		XMStoreFloat4x4(&dir.toWorldMatrix, XMMatrixTranspose(lightmapDirectories[i].toWorldMatrix));

		// Store vertices
		Vector3 tri[3];
		scene.getTribyGlobalIndexFast(tri, i);
		dir.vertices[0] = XMFLOAT3(tri[0].x, tri[0].y, tri[0].z);
		dir.vertices[1] = XMFLOAT3(tri[1].x, tri[1].y, tri[1].z);
		dir.vertices[2] = XMFLOAT3(tri[2].x, tri[2].y, tri[2].z);

		XMVECTOR plane = XMPlaneFromPoints(tri[0], tri[1], tri[2]);
		XMStoreFloat4(&dir.plane, plane);

		finalDirectoryBuffer.push_back(dir);
	}

	// Time to pack the RDFs
	for (int i = 0; i < globalPolyCount; i++) {

		SurfaceLightmapDirectory curddDir = lightmapDirectories[i];
		vector<SurfLight> surfLights = {};

		for (int j : lightmapDirectories[i].surfLights) {
			RDF surfLightUnpacked = jumbleMap[j];
			SurfLight surfLightPacked = {};
			SurfaceLightmapDirectory parentDir;

			// If we are at a lightmap root we can ignore it
			if (surfLightUnpacked.parentRDF == -1) {
				continue;
			}

			int parentRDFidx = surfLightUnpacked.parentRDF;
			int parentRDFdirIdx = jumbleMap[parentRDFidx].parentDirectoryIndex;
			parentDir = lightmapDirectories[parentRDFdirIdx];

			surfLightPacked.casterIdx = parentRDFdirIdx;
			// colour will be later
			surfLightPacked.lightBrightness = surfLightUnpacked.lightBrightness;
			// shadows are later

			surfLights.push_back(surfLightPacked);
		}

		finalLightmapBuffer[i] = surfLights;
	}
}

std::vector<SurfaceLightmapDirectoryPacked> SceneLightingInformation::GetDirectoryBuffer() {
	return finalDirectoryBuffer;
}

std::map<int, std::vector<SurfLight>> SceneLightingInformation::GetFinalLightmapBuffer() {
	return finalLightmapBuffer;
}