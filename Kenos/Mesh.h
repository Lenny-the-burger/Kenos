#pragma once

#include <vector>
#include <DirectXMath.h>

class Mesh
{
public:
	Mesh();
    Mesh(std::vector<DirectX::SimpleMath::Vector3> vertices, std::vector<DirectX::SimpleMath::Vector3> indices);
    ~Mesh();

    void SetVertices(const DirectX::SimpleMath::Vector3 vertices[]);
    const DirectX::SimpleMath::Vector3 GetVert(int idx) const;
    int GetVertexCount() const;

    void SetIndices(const DirectX::SimpleMath::Vector3 indices[]);
    const DirectX::SimpleMath::Vector3 GetIndex(int idx) const;
    int GetFaceCount() const;

private:
    std::vector<DirectX::SimpleMath::Vector3> m_vertices;
    std::vector<DirectX::SimpleMath::Vector3> m_indices;
};