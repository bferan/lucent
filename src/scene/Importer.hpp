#pragma once

#include "scene/EntityIDPool.hpp"
#include "scene/Scene.hpp"

namespace tinygltf
{
class Model;
class Node;
}

namespace lucent
{

class Device;

//! Imports entities, models and textures from a file
//! Currently supports gLTF binary and JSON formats
class Importer
{
public:
    explicit Importer(Device* device);

    Entity Import(Scene& scene, const std::string& modelFile);

    void Clear();

private:
    void ImportMaterials(Scene& scene, const tinygltf::Model& model);
    void ImportMeshes(Scene& scene, const tinygltf::Model& gltfModel);
    Entity ImportEntities(Scene& scene, const tinygltf::Model& model, const tinygltf::Node& node, Entity parent);

    std::vector<Model*> m_ImportedMeshes;
    std::vector<Material*> m_ImportedMaterials;
    std::string_view m_ModelFile;

    Device* m_Device;
};

}
