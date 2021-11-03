#pragma once

#include "scene/Scene.hpp"

namespace lucent
{

class View
{
public:
    void SetScene(Scene* scene);

    Scene& GetScene();

    void BindUniforms(Context& ctx);

private:
    Scene* m_Scene{};

};

}
