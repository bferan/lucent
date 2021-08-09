#pragma once

#include "assimp/scene.h"

#include "scene/EntityPool.hpp"
#include "scene/Scene.hpp"

namespace lucent
{

class Device;

class Importer
{
public:
    Importer(Device* device);

    Entity Import(Scene& scene, const std::string& modelFile);

    void Clear();

private:
    void ImportMaterials(Scene& scene, const aiScene& model);
    void ImportMeshes(Scene& scene, const aiScene& model);
    Entity ImportEntities(Scene& scene, const aiScene& model, const aiNode& node, Entity parent);

    std::vector<uint32> m_MeshIndices;
    std::vector<uint32> m_MaterialIndices;
    std::string_view m_ModelFile;

    Device* m_Device;
};

}
