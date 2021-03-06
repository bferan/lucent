#pragma once

namespace lucent
{

//! Model instance component
struct ModelInstance
{
    Model* model = nullptr;

    //! Material override; if null uses the default materials in the model
    Material* material = nullptr;
};

}