#pragma once

#include "device/Context.hpp"

namespace lucent
{

class Material
{
public:
    virtual ~Material() = default;

    virtual Material* Clone() = 0;

    virtual Pipeline* GetPipeline() = 0;

    virtual void BindUniforms(Context& context) {}
};

}
