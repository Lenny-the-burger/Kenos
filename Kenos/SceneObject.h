#pragma once

#include <memory>
#include <vector>
#include <DirectXMath.h>
#include <SimpleMath.h>

#include "Mesh.h"
#include "Material.h"

class SceneObject
{
public:
    SceneObject();
    ~SceneObject();

    void SetPosition(const DirectX::SimpleMath::Vector3 position);
    const DirectX::SimpleMath::Vector3& GetPosition() const;

    void SetRotation(const DirectX::SimpleMath::Vector3 rotation);
    const DirectX::SimpleMath::Vector3& GetRotation() const;

    void SetScale(const DirectX::SimpleMath::Vector3 scale);
    const DirectX::SimpleMath::Vector3& GetScale();

    void SetMesh(Mesh mesh);
    const Mesh& GetMesh();

    void SetMaterial(Material& material);
    const Material& GetMaterial();

    // get the final (transformed/scaled/rotated) vertex at index idx
    const DirectX::SimpleMath::Vector3 GetFinalVtx(int idx) const;

private:
    DirectX::SimpleMath::Vector3 m_position;
    DirectX::SimpleMath::Vector3 m_rotation;
    DirectX::SimpleMath::Vector3 m_scale;
    Mesh m_mesh;
    Material m_material;
};

