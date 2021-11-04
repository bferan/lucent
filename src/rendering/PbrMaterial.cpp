#include "rendering/PbrMaterial.hpp"

#include "device/Context.hpp"

namespace lucent
{

void PbrMaterial::BindUniforms(Context& ctx)
{
    ctx.BindTexture("u_BaseColor"_id, baseColorMap);
    ctx.BindTexture("u_MetalRoughness"_id, metalRough);
    ctx.BindTexture("u_Normal"_id, normalMap);

    ctx.Uniform("u_BaseColorFactor"_id, baseColorFactor);
    ctx.Uniform("u_MetallicFactor"_id, metallicFactor);
    ctx.Uniform("u_RoughnessFactor"_id, roughnessFactor);
}

PbrMaterial* PbrMaterial::Clone()
{
    // TODO: Implement
    return nullptr;
}

PbrMaterial::~PbrMaterial()
{

}

Pipeline* PbrMaterial::GetPipeline()
{
    return nullptr;
}

}