#pragma once

#include "pch.h"
#include <DirectXMath.h>
#include <SimpleMath.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

#include "Material.h"

Material::Material() {

}

Material::~Material() {

}

void Material::SetRoughness(float roughness) {
    m_roughness = roughness;
}

float Material::GetRoughness() const {
    return m_roughness;
}

void Material::SetAlbedo(const Color& albedo) {
    m_albedo = albedo;
}

const Color& Material::GetAlbedo() const {
    return m_albedo;
}

void Material::SetEmissiveIntensity(float emissiveIntensity) {
    m_emissiveIntensity = emissiveIntensity;
}

float Material::GetEmissiveIntensity() const {
    return m_emissiveIntensity;
}