#include "Scene.hpp"

namespace lucent
{

Entity Scene::CreateEntity()
{
    return Entity{ m_Entities.Create(), this };
}

void Scene::Destroy(Entity entity)
{
    for (auto& pool : m_ComponentPoolsByIndex)
    {
        if (pool && pool->Contains(entity.id))
            pool->Remove(entity.id);
    }
    m_Entities.Destroy(entity.id);
}

Entity Scene::Find(EntityID id)
{
    return Entity{ id, this };
}

Model* Scene::AddModel(std::unique_ptr<Model> model)
{
    return m_Models.emplace_back(std::move(model)).get();
}

Material* Scene::AddMaterial(std::unique_ptr<Material> material)
{
    return m_Materials.emplace_back(std::move(material)).get();
}

Material* Scene::GetDefaultMaterial() const
{
    return nullptr;
}

//// Create default material
//Material material{};
//material.baseColorMap = g_WhiteTexture;
//material.metalRough = g_GreenTexture;
//material.normalMap = g_NormalTexture;
//material.aoMap = g_WhiteTexture;
//material.emissive = g_BlackTexture;
//
//material.baseColorFactor = Color::Gray();
//material.metallicFactor = 0.0f;
//material.roughnessFactor = 1.0f;
//
//mesh.materialIdx = scene.materials.size();
//scene.materials.emplace_back(material);


}