#pragma once

#include "device/Context.hpp"

namespace lucent
{

//! Interface exposing a shader bound with specific textures and uniforms
// TODO: Shader interface is programmatic at the moment but could be made more data driven
class Material
{
public:
    virtual ~Material() = default;

    virtual Material* Clone() = 0;

    virtual Pipeline* GetPipeline() = 0;

    virtual void BindUniforms(Context& context) {}
};

}
