#include "pch.h"
#include <DirectXMath.h>
#include <SimpleMath.h>
#include <vector>

#include "Mesh.h"

using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Mesh::Mesh(vector<Vector3> vertices, vector<Vector3> indices)
{
	// Set member variables
	m_vertices = vertices;
	m_indices = indices;
    
}

Mesh::~Mesh()
{
    // Destructor implementation
}

void Mesh::SetVertices(const Vector3 vertices[])
{
    m_vertices.clear();

    // Assuming the vertices array is null-terminated
    for (int i = 0; vertices[i].x != 0.0f || vertices[i].y != 0.0f || vertices[i].z != 0.0f; ++i)
    {
        m_vertices.push_back(vertices[i]);
    }
}

const Vector3* Mesh::GetVertices() const
{
    return m_vertices.data();
}

int Mesh::GetVertexCount() const
{
    return static_cast<int>(m_vertices.size());
}

void Mesh::SetIndices(const Vector3 indices[])
{
    m_indices.clear();

    // Assuming the indices array is null-terminated
    for (int i = 0; indices[i].x != 0.0f || indices[i].y != 0.0f || indices[i].z != 0.0f; ++i)
    {
        m_indices.push_back(indices[i]);
    }
}

const Vector3* Mesh::GetIndices() const
{
    return m_indices.data();
}

int Mesh::GetFaceCount() const
{
    return static_cast<int>(m_indices.size());
}
