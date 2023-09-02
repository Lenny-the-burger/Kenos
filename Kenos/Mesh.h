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

    std::vector<DirectX::SimpleMath::Vector3> m_vertices;
    std::vector<DirectX::SimpleMath::Vector3> m_indices;

private:
    // The vertices and indeces should probably be private but i dont want to 
    // engage in pointer shenaningans right now for the sake of optimization
};