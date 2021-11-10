#pragma once

#include "rendering/Material.hpp"

namespace lucent
{

class PbrMaterial : public Material
{
public:
    PbrMaterial* Clone() override;

    ~PbrMaterial() override;

    Pipeline* GetPipeline() override;

    void BindUniforms(Context& context) override;

public:
    Color baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float emissiveFactor = 1.0f;

    Texture* baseColorMap;
    Texture* metalRough;
    Texture* normalMap;
    Texture* aoMap;
    Texture* emissive;
};



}