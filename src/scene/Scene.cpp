#include "Scene.hpp"
#include "rendering/PbrMaterial.hpp"
#include "rendering/Engine.hpp"

namespace lucent
{

Entity Scene::CreateEntity()
{
    return Entity{ m_Entities.Create(), this };
}

void Scene::Destroy(Entity entity)
{
    for (auto& pool: m_ComponentPoolsByIndex)
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

Material* Scene::GetDefaultMaterial()
{
    auto& settings = Engine::Instance()->GetRenderSettings();

    auto pbr = std::make_unique<PbrMaterial>();
    pbr->baseColorFactor = Color::Gray();
    pbr->metallicFactor = 0.0f;
    pbr->roughnessFactor = 1.0f;

    pbr->baseColorMap = settings.defaultWhiteTexture;
    pbr->metalRough = settings.defaultGreenTexture;
    pbr->normalMap = settings.defaultNormalTexture;
    pbr->aoMap = settings.defaultWhiteTexture;
    pbr->emissive = settings.defaultBlackTexture;

    return AddMaterial(std::move(pbr));
}

}