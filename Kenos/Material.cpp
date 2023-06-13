#pragma once

#include "pch.h"
#include <DirectXMath.h>
#include <SimpleMath.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

#include "Material.h"

Material::Material() 
    : m_roughness(0.0f), m_albedo(Colors::White), m_emissiveColor(Colors::Black), m_emissiveIntensity(0.0f) {
    // Initialize member variables
}

Material::~Material() {
    // Perform cleanup, if necessary
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