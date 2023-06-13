#pragma once

#include <DirectXMath.h>
#include <SimpleMath.h>

class Material
{
public:
    Material();
    ~Material();

    void SetRoughness(float roughness);
    float GetRoughness() const;

    void SetAlbedo(const DirectX::SimpleMath::Color& albedo);
    const DirectX::SimpleMath::Color& GetAlbedo() const;

    void SetEmissiveIntensity(float emissiveIntensity);
    float GetEmissiveIntensity() const;

private:
    float m_roughness;
    DirectX::SimpleMath::Color m_albedo;
    float m_emissiveIntensity;
};