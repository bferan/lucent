#include "Model.hpp"

namespace lucent
{

Model::Model(StaticMesh mesh, Material* material)
{
    AddMesh(std::move(mesh), material);
}

void Model::AddMesh(StaticMesh mesh, Material* material)
{
    m_Primitives.push_back(Primitive{std::move(mesh), material});
}

}
