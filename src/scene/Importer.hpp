#pragma once

#include "scene/EntityPool.hpp"
#include "scene/Scene.hpp"

namespace tinygltf
{
class Model;
class Node;
}

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
    void ImportMaterials(Scene& scene, const tinygltf::Model& model);
    void ImportMeshes(Scene& scene, const tinygltf::Model& model);
    Entity ImportEntities(Scene& scene, const tinygltf::Model& model, const tinygltf::Node& node, Entity parent);

    std::vector<MeshInstance> m_MeshInstances;
    std::vector<uint32> m_MaterialIndices;
    std::string_view m_ModelFile;

    Device* m_Device;
};

}
