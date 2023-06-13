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

SceneObject::SceneObject()
{
    // Initialize member variables
}

SceneObject::~SceneObject()
{
    // Perform cleanup, if necessary
}

void SceneObject::SetPosition(const Vector3& position)
{
    m_position = position;
}

const Vector3& SceneObject::GetPosition() const
{
    return m_position;
}

void SceneObject::SetRotation(const Vector3& rotation)
{
    m_rotation = rotation;
}

const Vector3& SceneObject::GetRotation() const
{
    return m_rotation;
}

void SceneObject::SetScale(const Vector3& scale)
{
    m_scale = scale;
}

Vector3 SceneObject::GetScale() const
{
    return m_scale;
}

void SceneObject::SetMesh(Mesh* mesh)
{
    m_mesh = mesh;
}

Mesh* SceneObject::GetMesh() const
{
    return m_mesh;
}

void SceneObject::SetMaterial(Material* material)
{
    m_material = material;
}

const Material* SceneObject::GetMaterial() const
{
    return m_material;
}

// get the final (transformed/scaled/rotated) vertex at index idx
const Vector3 SceneObject::GetFinalVtx(int idx) const
{
    // Retrieve the original vertex at the given index
    Vector3 vertex = m_mesh->GetVertices()[idx];

    // Apply transformations (scale, rotation, and translation)
    vertex *= m_scale;                          // Scale
    vertex = Vector3::Transform(vertex,         // Rotation
        Matrix::CreateFromYawPitchRoll(m_rotation.y, m_rotation.x, m_rotation.z));
    vertex += m_position;                       // Translation

    return vertex;
}

SceneObject getSceneObjectById(int id)
{
	return m_globalSceneObjects[id];
}