#pragma once

#include "pch.h"
#include <vector>
#include <SimpleMath.h>

#include "Mesh.h"
#include "Material.h"
#include "SceneObject.h"
#include "SceneInformation.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

#include "SceneObject.h"

SceneObject::SceneObject() {
	// init material reference
	
}

SceneObject::~SceneObject()
{
    // Perform cleanup, if necessary
}

void SceneObject::SetPosition(const Vector3 position)
{
    m_position = position;
}

const Vector3& SceneObject::GetPosition() const
{
    return m_position;
}

void SceneObject::SetRotation(const Vector3 rotation)
{
    m_rotation = rotation;
    m_rotQuat = XMQuaternionRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
}

const Vector3& SceneObject::GetRotation() const
{
    return m_rotation;
}

void SceneObject::SetScale(const Vector3 scale)
{
    m_scale = scale;
}

const Vector3& SceneObject::GetScale()
{
    return m_scale;
}

void SceneObject::SetMesh(Mesh mesh)
{
    m_mesh = mesh;
}

const Mesh& SceneObject::GetMesh()
{
    return m_mesh;
}

void SceneObject::SetMaterial(Material& material)
{
    m_material = material;
}

const Material& SceneObject::GetMaterial()
{
    return m_material;
}

int SceneObject::GetFaceCount() const{
    return m_mesh.GetFaceCount();
}

Vector3  SceneObject::GetMeshIndex(int idx) {
    return m_mesh.m_indices[idx];
}

// get the final (transformed/scaled/rotated) vertex at index idx
const Vector3 SceneObject::GetFinalVtx(int idx) const
{
    Vector3 vertex = m_mesh.GetVert(idx);

    // Apply transformations (scale, rotation, and translation)
    vertex *= m_scale;                          // Scale
    
	vertex = XMVector3Rotate(vertex, m_rotQuat);  // Rotation
    
    vertex += m_position;                       // Translation

    return vertex;
}

void SceneObject::computeBVH() {
    // loop through all the verts and find the max and min x, y, and z values
    int numVerts = m_mesh.GetVertexCount();

    Vector3 max = Vector3::Zero;
    Vector3 min = Vector3::Zero;

    for (int i = 0; i < numVerts; i++) {
        max = Vector3::Max(max, GetFinalVtx(i));
        min = Vector3::Min(min, GetFinalVtx(i));
    }

    m_BVHmax = max;
    m_BVHmin = min;
}

std::tuple<Vector3, Vector3> SceneObject::getBVH() const {
	return std::make_tuple(m_BVHmax, m_BVHmin);
}